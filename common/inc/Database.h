/**
 **	Database.h
 **
 **	Published / author: 2001-01-04 / grymse@telia.com
 **/

/*
Copyright (C) 2001  Anders Hedstrom

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef _DATABASE_H
#define _DATABASE_H


/**
 * Connection pool struct.
 * Database connection handle.
*/

typedef struct opendbstruct
{
	/** ptr to next member in linked list */
	struct opendbstruct *next;
	/** MySQL connection handle */
	MYSQL mysql;
	/** Connection busy flag */
	short busy;
} OPENDB;


/**
 * Session control.
 * Owns and maintains a list of active connections.
*/

class Database {
public:
	/** Constructor which does nothing.
	*/
	Database(void) {
		host = user = password = database = NULL;
		opendbbase = NULL;
		errc = 0;
	}
	/**
	 * The real constructor for this class.
	 * @param h Host
	 * @param u User
	 * @param p Password
	 * @param d Database
	*/
	Database(char *h,char *u,char *p,char *d) {
		host = user = password = database = NULL;
		opendbbase = NULL;
		errc = 0;

		host = new char[strlen(h) + 1];
		strcpy(host,h);
		user = new char[strlen(u) + 1];
		strcpy(user,u);
		password = new char[strlen(p) + 1];
		strcpy(password,p);
		database = new char[strlen(d) + 1];
		strcpy(database,d);
		
		freedb(grabdb());		// open one connection
	}
	/** Destructor will warn on stderr if there are active connections
	*/
	~Database(void) {
		OPENDB *odb;
//printf("~Database()\n");
		if (host)
			delete host;
		if (user)
			delete user;
		if (password)
			delete password;
		if (database)
			delete database;
		for (odb = opendbbase; odb; odb = odb -> next)
			mysql_close(&odb -> mysql);
		while (opendbbase)
		{
			odb = opendbbase;
			opendbbase = opendbbase -> next;
			if (odb -> busy)
				fprintf(stderr,"destroying Database object before Connect object(s)\n");
			delete odb;
		}
	}
	/**
	 * Get an active & free database connection.
	 * @return Database connection handle
	*/
	OPENDB *grabdb(void) {
		OPENDB *odb;
		for (odb = opendbbase; odb; odb = odb -> next)
			if (!odb -> busy)
				break;
		if (!odb)
		{
			odb = new OPENDB;
			if (!mysql_init(&odb -> mysql))
			{
				fprintf(stderr,"mysql_init() failed\n");
				errc = 1;
			}

			if (!mysql_real_connect(&odb -> mysql, host, user, password, "zangsi", 0, NULL, 0))
			{
				fprintf(stderr,"mysql_connect(%s,%s,***) failed\n",host,user);
				errc = 2;
			}
/*

			if (!mysql_connect(&odb -> mysql,host,user,password))
			{
				fprintf(stderr,"mysql_connect(%s,%s,***) failed\n",host,user);
				errc = 2;
			}
*/
			if (mysql_select_db(&odb -> mysql,database))
			{
				fprintf(stderr,"mysql_select_db(%s,%s,%s,%s) failed\n",host,user,password,database);
				errc = 3;
			}
			odb -> busy = 1;
			odb -> next = opendbbase;
			opendbbase = odb;
		} else
			odb -> busy++;
		return odb;
	}
	/**
	 * Release (but not close) a database connection.
	 * @param odb Database connection handle
	*/
	void freedb(OPENDB *odb) {
		odb -> busy = 0;
	}
	short errcode(void) {
		return errc;
	}
	/** @param val Enable debugging if 1 */
	void debug(short val) { _debug = val; }
	short debug(void) { return _debug; }
private:
	char *host;
	char *user;
	char *password;
	char *database;
	OPENDB *opendbbase;
	short errc;
	short _debug;
};



#endif // _DATABASE_H
