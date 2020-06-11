#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "mysqlpool.h"


////////////////////////////////////////////////////////////
// 功能：创建连接池
// 输入：连接池节点数，数据库地址、端口、名称、用户、密码
// 输出：
// 返回：连接池地址
////////////////////////////////////////////////////////////
SQL_CONN_POOL* sql_pool_create(int sql_conn_count, char ip[], int port, char database[], char user[], char passwd[])
{
	if (sql_conn_count < 1) return NULL;
	
	SQL_CONN_POOL* sql_conn_pool = NULL;
	sql_conn_pool = (SQL_CONN_POOL*)malloc(sizeof(SQL_CONN_POOL));
	if (sql_conn_pool == NULL) return NULL;

	sql_conn_pool->shutdown = 0;
	sql_conn_pool->conn_count = 0;
	sql_conn_pool->busy_count = 0;
	strcpy(sql_conn_pool->ip, ip);
	sql_conn_pool->port = port;
	strcpy(sql_conn_pool->database, database);
	strcpy(sql_conn_pool->user, user);
	strcpy(sql_conn_pool->passwd, passwd);


	if (sql_conn_count > SQL_CONN_COUNT_MAX) {
		sql_conn_count = SQL_CONN_COUNT_MAX;
	}

	int i;
	for (i = 0; i < sql_conn_count; i++) {

		if (0 != sql_conn_create(sql_conn_pool, &sql_conn_pool->conn_pool[i])) {
			sql_pool_destroy(sql_conn_pool);
			return NULL;
		}

		sql_conn_pool->conn_pool[i].index = i;
		sql_conn_pool->conn_count++;	
	}

	return sql_conn_pool;
}

////////////////////////////////////////////////////////////
// 功能：销毁连接池
// 输入：连接池地址
// 输出：
// 返回：无
////////////////////////////////////////////////////////////
void sql_pool_destroy(SQL_CONN_POOL* sql_conn_pool)
{
	sql_conn_pool->shutdown = 1;
	int i;
	int sql_conn_count = sql_conn_pool->conn_count;
	for (i = 0; i < sql_conn_count; i++) {

		if (NULL != sql_conn_pool->conn_pool[i].mysql_sock) {
			mysql_close(sql_conn_pool->conn_pool[i].mysql_sock);
			mysql_library_end();
			sql_conn_pool->conn_pool[i].mysql_sock = NULL;
		}
		sql_conn_pool->conn_pool[i].sql_state = DB_DISCONN;
		sql_conn_pool->conn_count--;
	}
	free(sql_conn_pool);
	sql_conn_pool = NULL;
}

////////////////////////////////////////////////////////////
// 功能：扩展连接池
// 输入：连接池地址，扩展量
// 输出：
// 返回：无
////////////////////////////////////////////////////////////
void sql_pool_expand(SQL_CONN_POOL* sql_conn_pool, int count)
{
	int i;
	int sql_conn_count = sql_conn_pool->conn_count;
	for (i = sql_conn_count; i < sql_conn_count + count; i++) {

		if (0 != sql_conn_create(sql_conn_pool, &sql_conn_pool->conn_pool[i])) {
			sql_pool_destroy(sql_conn_pool);
		}
		sql_conn_pool->conn_pool[i].index = i;   
		sql_conn_pool->conn_count++;
	}
}

////////////////////////////////////////////////////////////
// 功能：压缩连接池
// 输入：连接池地址，压缩量
// 输出：
// 返回：无
////////////////////////////////////////////////////////////
void sql_pool_reduce(SQL_CONN_POOL* sql_conn_pool, int count)
{
	int i;
	int sql_conn_count = sql_conn_pool->conn_count;
	for (i = sql_conn_count - 1; i > (sql_conn_count - count - 1) && i >= 0; i--) {

		if (NULL != sql_conn_pool->conn_pool[i].mysql_sock) {
			mysql_close(sql_conn_pool->conn_pool[i].mysql_sock);
			mysql_library_end();
			sql_conn_pool->conn_pool[i].mysql_sock = NULL;
		}
		sql_conn_pool->conn_pool[i].sql_state = DB_DISCONN;
		sql_conn_pool->conn_count--;
	}
}

////////////////////////////////////////////////////////////
// 功能：创建连接
// 输入：连接池地址，连接节点地址
// 输出：
// 返回：0-成功 -1-失败
////////////////////////////////////////////////////////////
int sql_conn_create(SQL_CONN_POOL* sql_conn_pool, SQL_NODE* sql_node)
{
	if (sql_conn_pool->shutdown == 1) return -1;
	if (NULL == mysql_init(&sql_node->fd)) return -1;

	sql_node->mysql_sock = mysql_real_connect(&sql_node->fd, sql_conn_pool->ip, sql_conn_pool->user, 
		sql_conn_pool->passwd, sql_conn_pool->database, sql_conn_pool->port, NULL, 0);
	if (!sql_node->mysql_sock) {
		sql_node->sql_state = DB_DISCONN;
		return -1;
	}

	pthread_mutex_init(&sql_node->lock, NULL);
	sql_node->used = 0;
	sql_node->sql_state = DB_CONN;

	int options = 1;
	mysql_options(&sql_node->fd, MYSQL_OPT_RECONNECT, &options);
	options = 3;
	mysql_options(&sql_node->fd, MYSQL_OPT_CONNECT_TIMEOUT, &options);

	return 0;
}

////////////////////////////////////////////////////////////
// 功能：取用连接
// 输入：连接池地址
// 输出：
// 返回：连接节点地址
////////////////////////////////////////////////////////////
SQL_NODE* sql_conn_get(SQL_CONN_POOL* sql_conn_pool)
{
	if (sql_conn_pool->shutdown == 1) return NULL;

	srand((int)time(0));
	int start_index = rand() % sql_conn_pool->conn_count;

	int i, index;
	for (i = 0; i < sql_conn_pool->conn_count; i++) {

		index = (start_index + i) % sql_conn_pool->conn_count;
		if (pthread_mutex_trylock(&sql_conn_pool->conn_pool[index].lock)) {
			continue;
		}

		if (DB_DISCONN == sql_conn_pool->conn_pool[index].sql_state) {
			if (0 != sql_conn_create(sql_conn_pool, &(sql_conn_pool->conn_pool[index]))) {
				sql_conn_release(sql_conn_pool, &(sql_conn_pool->conn_pool[index]));
				continue;
			}
		}

		int ping_res = mysql_ping(sql_conn_pool->conn_pool[index].mysql_sock);
		if (0 != ping_res) {
			sql_conn_pool->conn_pool[index].sql_state = DB_DISCONN;
			sql_conn_release(sql_conn_pool, &(sql_conn_pool->conn_pool[index]));
			continue;
		}
		else {
			sql_conn_pool->conn_pool[index].used = 1;
			sql_conn_pool->busy_count++;
			break;
		}
	}

	return i == sql_conn_pool->conn_count ? NULL : &(sql_conn_pool->conn_pool[index]);
}

////////////////////////////////////////////////////////////
// 功能：释放连接
// 输入：连接池地址，连接节点地址
// 输出：
// 返回：无
////////////////////////////////////////////////////////////
void sql_conn_release(SQL_CONN_POOL* sql_conn_pool, SQL_NODE* sql_node)
{
	sql_node->used = 0;
	sql_conn_pool->busy_count--;
	pthread_mutex_unlock(&sql_node->lock);
}
