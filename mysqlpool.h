#ifndef MYSQLPOOL_H
#define MYSQLPOOL_H

#include <mysql/mysql.h>


#define SQL_CONN_COUNT_MAX		20


////////////////////////////////////////////////////////////
// 连接节点数据结构
////////////////////////////////////////////////////////////
typedef struct _SQL_NODE
{
	MYSQL fd;								// MYSQL对象文件描述符
	MYSQL* mysql_sock;						// 指向已经连接的MYSQL的指针
	pthread_mutex_t lock;					// 互斥锁
	int used;								// 使用标志
	int index;								// 下标
	enum {
		DB_DISCONN, DB_CONN
	}sql_state;

}SQL_NODE;

////////////////////////////////////////////////////////////
// 连接池数据结构
////////////////////////////////////////////////////////////
typedef struct _SQL_CONN_POOL
{
	SQL_NODE conn_pool[SQL_CONN_COUNT_MAX];	// 一堆连接
	int shutdown;							// 是否关闭
	int conn_count;							// 连接数量
	int busy_count;							// 被获取了的连接数量
	char ip[16];							// 数据库地址
	int port;								// 数据库端口
	char database[64];						// 数据库名字
	char user[64];							// 数据库用户
	char passwd[64];						// 数据库密码

}SQL_CONN_POOL;


// 创建连接池
SQL_CONN_POOL* sql_pool_create(int sql_conn_count, char ip[], int port, char database[], char user[], char passwd[]);
// 销毁连接池
void sql_pool_destroy(SQL_CONN_POOL* sql_conn_pool);
// 扩展连接池
void sql_pool_expand(SQL_CONN_POOL* sql_conn_pool, int count);
// 压缩连接池
void sql_pool_reduce(SQL_CONN_POOL* sql_conn_pool, int count);
// 创建连接
int sql_conn_create(SQL_CONN_POOL* sql_conn_pool, SQL_NODE* sql_node);
// 取用连接
SQL_NODE* sql_conn_get(SQL_CONN_POOL* sql_conn_pool);
// 释放连接
void sql_conn_release(SQL_CONN_POOL* sql_conn_pool, SQL_NODE* sql_node);


#endif // !MYSQLPOOL_H
