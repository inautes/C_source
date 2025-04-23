/*
 * A simple example client, using the embedded MySQL server library
 */

#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

MYSQL *db_connect(const char *dbname);
void db_disconnect(MYSQL *db);
void db_do_query(MYSQL *db, const char *query);

const char *server_groups[] = {
	"test_libmysqld_SERVER", "embedded", "server", NULL
};

int test_main()
{
	MYSQL *one, *two;

	/* mysql_server_init() must be called before any other mysql
	* functions.
	*
	* You can use mysql_server_init(0, NULL, NULL), and it will
	* initialize the server using groups = {
	*   "server", "embedded", NULL
	*  }.
	*
	* In your $HOME/.my.cnf file, you probably want to put:

	[test_libmysqld_SERVER]
	language = /path/to/source/of/mysql/sql/share/english

	* You could, of course, modify argc and argv before passing
	* them to this function.  Or you could create new ones in any
	* way you like.  But all of the arguments in argv (except for
	* argv[0], which is the program name) should be valid options
	* for the MySQL server.
	*
	* If you link this client against the normal mysqlclient
	* library, this function is just a stub that does nothing.
	*/
/*
	mysql_server_init(argc, argv, (char **)server_groups);
*/
	
	one = db_connect("zangsi");
	
	
	two = db_connect(NULL);

	db_do_query(one, "SHOW TABLE STATUS");
	db_do_query(two, "SHOW DATABASES");

	mysql_close(two);
	mysql_close(one);

	/* This must be called after all other mysql functions */
	mysql_server_end();

	return(EXIT_SUCCESS);
}

static void die(MYSQL *db, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)putc('\n', stderr);
	if (db)
		db_disconnect(db);
	exit(EXIT_FAILURE);
}

MYSQL *db_connect(const char *dbname)
{
	MYSQL *db = mysql_init(NULL);
	if (!db)
		die(db, "mysql_init failed: no memory");
	/*
	 * Notice that the client and server use separate group names.
	 * This is critical, because the server will not accept the
	 * client's options, and vice versa.
	 */
	mysql_options(db, MYSQL_READ_DEFAULT_GROUP, "test_libmysqld_CLIENT");
/*
	if (!mysql_real_connect(db, NULL, NULL, NULL, dbname, 0, NULL, 0))
	die(db, "mysql_real_connect failed: %s", mysql_error(db));
*/
	if (!mysql_real_connect(db, "192.168.0.5", "root", "qaz741", dbname, 0, NULL, 0))
		die(db, "mysql_real_connect failed: %s", mysql_error(db));

	return db;
}

void db_disconnect(MYSQL *db)
{
	mysql_close(db);
}

void db_do_query(MYSQL *db, const char *query)
{
	if (mysql_query(db, query) != 0)
		goto err;

	if (mysql_field_count(db) > 0)
	{
		MYSQL_RES   *res;
		MYSQL_ROW    row, end_row;
		int num_fields;

		if (!(res = mysql_store_result(db)))
			goto err;
		num_fields = mysql_num_fields(res);
		while ((row = mysql_fetch_row(res)))
		{
			(void)fputs(">> ", stdout);
			for (end_row = row + num_fields; row < end_row; ++row)
				(void)printf("%s\t", row ? (char*)*row : "NULL");
			(void)fputc('\n', stdout);
		}
		(void)fputc('\n', stdout);
		mysql_free_result(res);
	}
	else
		(void)printf("Affected rows: %lld\n", mysql_affected_rows(db));

	return;
err:
	die(db, "db_do_query failed: %s [%s]", mysql_error(db), query);
}
