#include <stdio.h>
#include "mysqlpool.h"


void main()
{
	SQL_CONN_POOL* sp = sql_pool_create(10, "localhost", 3306, "mysql", "root", "MYSQLlx&gw7!");
	printf("=========================\n");
	printf("conn_count: --%d--\n", sp->conn_count);
	printf("busy_count: --%d--\n", sp->busy_count);


	SQL_NODE* node1 = sql_conn_get(sp);
	if (NULL == node1) {
		printf("get sql pool node error.\n");
		return;
	}
	printf("=========================\n");
	printf("conn_count: --%d--\n", sp->conn_count);
	printf("busy_count: --%d--\n", sp->busy_count);
	printf("used_index: --%d--\n", node1->index);


	SQL_NODE* node2 = sql_conn_get(sp);
	if (NULL == node2) {
		printf("get sql pool node error.\n");
		return;
	}
	printf("=========================\n");
	printf("conn_count: --%d--\n", sp->conn_count);
	printf("busy_count: --%d--\n", sp->busy_count);
	printf("used_index: --%d--\n", node2->index);


	printf("=========================\n");
	if (mysql_query(&(node1->fd), "select User, Host from user")) {
		printf("sql node --%d-- query error.\n", node1->index);  					
	}
	else {
		printf("sql node --%d-- query succeed.\n", node1->index);
	}


	printf("=========================\n");
	if (mysql_query(&(node2->fd), "select User, Host from user")) {
		printf("sql node --%d-- query error.\n", node2->index);
		return;
	}
	else {
		printf("sql node --%d-- query succeed.\n", node2->index);
	}


	sql_conn_release(sp, node1);
	printf("=========================\n");
	printf("conn_count: --%d--\n", sp->conn_count);
	printf("busy_count: --%d--\n", sp->busy_count);
	printf("released_index: --%d--\n", node1->index);


	sql_conn_release(sp, node2);
	printf("=========================\n");
	printf("conn_count: --%d--\n", sp->conn_count);
	printf("busy_count: --%d--\n", sp->busy_count);
	printf("released_index: --%d--\n", node2->index);


	sql_pool_reduce(sp, 5);
	printf("=========================\n");
	printf("conn_count: --%d--\n", sp->conn_count);
	printf("busy_count: --%d--\n", sp->busy_count);
	printf("reduce_count: --%d--\n", 5);


	sql_pool_expand(sp, 5);
	printf("=========================\n");
	printf("conn_count: --%d--\n", sp->conn_count);
	printf("busy_count: --%d--\n", sp->busy_count);
	printf("expand_count: --%d--\n", 5);


	sql_pool_destroy(sp);
	printf("=========================\n");
	printf("conn_count: --%d--\n", sp->conn_count);
	printf("busy_count: --%d--\n", sp->busy_count);
}