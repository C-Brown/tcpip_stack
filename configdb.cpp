/*
 * =====================================================================================
 *
 *       Filename:  configdb.cpp
 *
 *    Description:  This file defines the routines and structures for config commit in postresql db
 *
 *        Version:  1.0
 *        Created:  01/28/2023 10:31:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#include <ctype.h>
#include <assert.h>
#include "PostgresLibpq/postgresLib.h"
#include "graph.h"

void 
node_config_db_init (node_t *node) {

    int i = 0;
    PGconn* conn;
    PGresult* sql_query_result;
    unsigned char db_name[128];
    unsigned char user_name[128];
    unsigned char sql_query[256];
    unsigned char node_name[NODE_NAME_SIZE];

    if (node->conn) return;

    assert ( (NODE_NAME_SIZE + strlen ("dbuser")) <= sizeof(user_name));
    assert ( (NODE_NAME_SIZE + strlen ("db")) <= sizeof(db_name));

    memset (db_name, 0, sizeof(db_name));
    memset (sql_query, 0, sizeof(sql_query));
    memset (user_name, 0, sizeof(user_name));
    memset (node_name, 0, sizeof(node_name));

    do {
        node_name[i] = tolower (node->node_name[i]);
        i++;
    } while (i < NODE_NAME_SIZE);

    node_name[NODE_NAME_SIZE - 1] = '\0';

    /* Create new DB user <node-name>-dbuser*/
    snprintf (user_name, sizeof(user_name),  "%sdbuser", node_name );

    assert(postgresql_create_new_user(NULL, user_name) != PGSQL_FAILED);

    /* Establish database connection for user <node-name>-dbuser */
    conn = postgres_get_user_connection(NULL,  "postgres");
    assert(conn);

    /* Create new DB - <node-name>-db*/
    snprintf (db_name, sizeof(db_name), "%sdb", node_name);
    assert(postgresql_create_new_database(NULL, db_name) != PGSQL_FAILED);

    /* Assigned DB <node-name>-db to user <node-name>-dbuser with all privileges*/
    assert (postgresql_database_assign_user (conn, user_name, db_name) != PGSQL_FAILED);

  /* Get rid of the superuser 'postgres' connection, re-establish the new connection for new user 
        with db-name also included as a paramater*/
    PQfinish(conn);
    snprintf (sql_query, sizeof(sql_query), 
                 "host=localhost user=%s dbname=%s password=%s",  user_name, db_name, user_name);

    node->conn = PQconnectdb(sql_query);

    assert(PQstatus(node->conn) == CONNECTION_OK);

    /* Now Delete Table <node-name>-config if exist*/
    snprintf (sql_query, sizeof(sql_query), 
                    "select tablename from  pg_catalog.pg_tables where tablename = '%sconfig' ",
                    node_name);
    
    sql_query_result = PQexec(node->conn, sql_query);

    if (PQntuples(sql_query_result) == 1) {

        /* Table Exist already, drop the table*/
        PQclear(sql_query_result);
        snprintf (sql_query, sizeof(sql_query), "drop table %sconfig ", node_name);
        sql_query_result = PQexec(node->conn, sql_query);
        assert (PQresultStatus(sql_query_result) == PGRES_COMMAND_OK);
    }
    PQclear(sql_query_result);
    
      /* Create new table*/
    snprintf (sql_query, sizeof(sql_query),
                    "create table %sconfig ( statement CHAR(256) PRIMARY KEY NOT NULL )", 
                    node_name);

    sql_query_result = PQexec(node->conn, sql_query);
    assert (PQresultStatus(sql_query_result) == PGRES_COMMAND_OK);
    PQclear(sql_query_result);
}


