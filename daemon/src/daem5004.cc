/******************************************************************************
 *   ¼­ºê½ýºÅÛ : daemonÇw¿¼½º
 *   Çw¿¿¥¸í : daem5004
 *         ±â´É : »èfµÈ ÄÁÅÙÃ÷ µðºñ µ¥ÀÌÅÍ delete ó¸®
 *         ¼³¸í : 1. Çw¿¼½º ó¸® ¿¹d½ð£ - 04:00 ÀÌÈÄ daem5002 ¿¿á ÀÌÈ¿¡ ¼öÇà.
 *                2. »èf±âÁØ - T_CONTENTS_ADMDELÀÇ µî·ÏÀÏÀ¿¡ ó¸®ÀÏÀÚ -3ÀÎ »èf°Ç
 *     ¼³¿'¿ : CMD01
 *
 *       À¿ºÀÚ : JDP
 *       À¿ºÀÏ : 2004/02/16
 *     ¼ödÀ¿Â : ¼ödÀÚ   - HCS.
 				  ¼öd³»¿ë - »èfµÈ À¿á Ç¿¿ º¸°ü. 
 				  ¼ödÀÏÀÚ - 20071204.
********************************************************************************
1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
*******************************************************************************/
#include <mysql.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>


#include "daemcom.h"
#include "commydb.h"

#define  MAX_ROWS	1
#define _DEBUG_

int    daem5004_init_process(int argc, char **argv);
int    daem5004_main_process();
int    daem5004_term_process();
int    daem5004_select_admdel();
int    daem5004_delete_data();
int    daem5004_delete_ask();
void   daem5004_signal(int nSignal);

MYSQL     *con;
MYSQL     *con_bck;
MYSQL_RES *res;
MYSQL_ROW  row;

char   gproc_date [  8+1];	// ó¸®ÀÏÀÚ
char   gdel_date1 [  8+1];	// »èf¿¹dÀÏÀÚ
char   gdel_date2 [  8+1];	// »èf¿¹dÀÏÀÚ

char   gcont_gu   [  2+1];	// ÄÁÅÙÃ¿Ð
unsigned long gid     = 0;  // µî·ÏID

char gadmdel_proc_date [9];
//******************************************************************************
//* daem5004 main
//******************************************************************************
int daem5004_main_process()
{
	char szQuery[1000];  // query string
	int i, j, count;
	int nRowcnt=0;
	int ret = 0;

    ZzLOG(ALWAY, "---------------------------------------------------------\n");
    ZzLOG(ALWAY, "daem5004_main_process: gproc_date[%s]\n", gproc_date);  
    ZzLOG(ALWAY, "daem5004_main_process: gdel_date1[%s]\n", gdel_date1);  
    ZzLOG(ALWAY, "daem5004_main_process: gdel_date2[%s]\n", gdel_date2);  
    ZzLOG(ALWAY, "---------------------------------------------------------\n");

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*  - T_CONTENTS_ADMDEL ±âÁØ8·Î ó¸®                                     */
	/*------------------------------------------------------------------------*/
	MYSQL_RES *res_sel;
	MYSQL_ROW  row_sel;	
	
	while(1)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "select min(reg_date) from zangsi.T_CONTENTS_ADMDEL where proc_yn in( 'N' ,'T') ; /*daem5004*/ "); // °ü¸®ÀÚ µðºñ v¿
		ZzLOG(ALWAY, "daem5004_select_admdel: [ %s ] \n",szQuery);
		if (mysql_query(con_bck, szQuery)){
		    ZzLOG(ERROR, "daem5004_select_admdel: mysql_query error...\n");
			ZzLOG(ERROR, "daem5004_select_admdel: [%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));

			ZzLOG(ALWAY, "reconnect \n");

			db_disconnect(con_bck);
		    if (!(con_bck=db_connect_backup("zangsi")))
		    {
		       ZzLOG(ERROR, "main bck DB¿¡ b¼ÓÇÏÁö ¸ø Ç¿´½4¿Ù...\n");
			   break;
			}

			memset(szQuery, 0x00, sizeof(szQuery));
			strcpy(szQuery, "select min(reg_date) from zangsi.T_CONTENTS_ADMDEL where proc_yn in( 'N' , 'T' ) ; /*daem5004*/ "); // °ü¸®ÀÚ µðºñ v¿
			ZzLOG(ALWAY, "daem5004_select_admdel rescan: [ %s ] \n",szQuery);

			if (mysql_query(con_bck, szQuery)){
		    	ZzLOG(ERROR, "daem5004_select_admdel: mysql_query error...\n");
				ZzLOG(ERROR, "daem5004_select_admdel: [%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));
				break;
			}
										
		}
		if (!(res_sel = mysql_store_result(con_bck))) {
		    ZzLOG(ERROR, "daem5004_select_admdel: mysql_store_result error...\n");
			ZzLOG(ERROR, "daem5004_select_admdel: [%d](%s)\n",mysql_errno(con_bck), mysql_error(con_bck));
			break;
		}
	 	if (mysql_num_rows(res_sel)==0)	{
			mysql_free_result(res_sel);
			break;
		}
	
		
		
		memset(gadmdel_proc_date, 0x00, sizeof(gadmdel_proc_date));
			
		if ( row_sel = mysql_fetch_row(res_sel))
		{
			strcpy(gadmdel_proc_date  , getstr(row_sel, 0));
			ZzLOG(ALWAY, "daem5004_select_admdel: [ %s ] \n",gadmdel_proc_date);
			if(strcmp(gadmdel_proc_date, gdel_date1) > 0)
			{
				ZzLOG(ALWAY, "daem5004_select_admdel: gadmdel_proc_date : [%s] gdel_date1 : [%s]\n", gadmdel_proc_date, gdel_date1);
				mysql_free_result(res_sel);
				ZzLOG(ALWAY, "daem5004_main_process: Á¤»óÀûÀ¸·Î Ã³¸® ¿Ï·á(%d)!!!\n", nRowcnt);
				break;
				//return 9;
			}
			else
			{
				while(1) {
		
					//----------------------------------------------------------------------
					// Æ®·»Á§¼Ç½ÃÀÛ
					//----------------------------------------------------------------------
					if (tran_begin(con)!=0) {
					    ZzLOG(ERROR, "daem5004_main_process: tran_begin: Å×ÀÌº£ÀÌ½º ¿À·ùÀÔ´Ï´Ù.\n");  
						ZzLOG(ERROR, "daem5004_main_process: [%d](%s)\n", mysql_errno(con), mysql_error(con));
					    ret = -1;
						break;
					}
					//----------------------------------------------------------------------
					// Ã³¸®ÇÒ Å×ÀÌÅ¸ 1°Ç select
					//----------------------------------------------------------------------
					ret = daem5004_select_admdel();
					if (ret ==  9) break;	// Ã³¸®ÇÒ ÀÚ·á°¡ ¾ø½À´Ï´Ù.
					if (ret == -1) break;	// ¿À·ù¹ß»ý
			
					//----------------------------------------------------------------------
					// ÀÚ·á½Ç »èÁ¦Ã³¸®
					//----------------------------------------------------------------------
					ret = daem5004_delete_data();
					if (ret == -1) 
					{
						ZzLOG(ERROR, "[daem5004_delete_data] gid:[%ld] gcont_gu:[%s] error...\n", gid, gcont_gu);
						break;	// ¿À·ù¹ß»ý
					}
					if (ret == 1) continue;
					//----------------------------------------------------------------------
					// commit
					//----------------------------------------------------------------------
					if (commit(con)!=0){
					    ZzLOG(ERROR, "[daem5004_main_process] commit 1 error...\n");
						ZzLOG(ERROR, "[daem5004_main_process] [%d](%s)\n", mysql_errno(con), mysql_error(con));
					    ret = -1;
						break;
					}
					nRowcnt++;
					ZzLOG(ALWAY, "daem5004_main_process: %ld »èÁ¦ (%d)!!!\n", gid, nRowcnt);				
				}		
				if (ret == -1) {
					tran_rollback(con);
					tran_end(con);
					return -1;	
				}
			}
			
		}
		mysql_free_result(res_sel);
	}
	
		
		
	

	/*------------------------------------------------------------------------*/
	/* Main loop                                                              */
	/*  - ÀÚ·á¿äÃ»¿¡ ´ëÇÑ Ã³¸®                                                */
	/*------------------------------------------------------------------------*/
	/*
	while(1) {
		
		//----------------------------------------------------------------------
		// Æ®·»Á§¼Ç½ÃÀÛ
		//----------------------------------------------------------------------
		if (tran_begin(con)!=0) {
		    ZzLOG(ERROR, "daem5004_main_process: tran_begin: Å×ÀÌº£ÀÌ½º ¿À·ùÀÔ´Ï´Ù.\n");  
			ZzLOG(ERROR, "daem5004_main_process: [%d](%s)\n", mysql_errno(con), mysql_error(con));
		    ret = -1;
			break;
		}

		//----------------------------------------------------------------------
		// ÀÚ·á½Ç »èÁ¦Ã³¸®
		//----------------------------------------------------------------------
		ret = daem5004_delete_ask();
		if (ret ==  9) break;	// Ã³¸®ÇÒ ÀÚ·á°¡ ¾ø½À´Ï´Ù.
		if (ret == -1) break;	// ¿À·ù¹ß»ý


		//----------------------------------------------------------------------
		// commit
		//----------------------------------------------------------------------
		if (commit(con)!=0){
		    ZzLOG(ERROR, "[daem5004_main_process] commit 1 error...\n");
			ZzLOG(ERROR, "[daem5004_main_process] [%d](%s)\n", mysql_errno(con), mysql_error(con));
		    ret = -1;
			break;
		}
	}		
	if (ret == -1) {
		tran_rollback(con);
		tran_end(con);
		return -1;	
	}
	*/

    ZzLOG(ALWAY, "daem5004_main_process: Á¤»óÀûÀ¸·Î Ã³¸® ¿Ï·á(%d)!!!\n", nRowcnt);
	return  0;
}


//******************************************************************************
//* daem5004_select_admdel
//*-----------------------------------------------------------------------------
//* ¼³¸í : Ã³¸®ÀÏÀÚ -7ÀÇ T_CONTENTS_ADMDELÀÇ ÀÚ·á¸¦ select ÇÑ´Ù.
//******************************************************************************
int daem5004_select_admdel()
{
	char szQuery[1000];		// query string
	//--------------------------------------------------------------------------
	// µð½ºÅ© Á¾·áÀÏÀÚ°¡ Ã³¸®ÀÏÀÚ -4ÀÏ ÀÎ ÀÚ·áÁß 1°ÇÀ» selectÇÑ´Ù.
	//--------------------------------------------------------------------------
	
	
	
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT a.cont_gu   , a.id        "
	                 "  FROM zangsi.T_CONTENTS_ADMDEL a"
	                 " WHERE a.reg_date = '%s' "
	                 "   AND a.proc_yn   in( 'N' ,'T')  "
	                 "  LIMIT 1  ; /*daem5004*/	"
                     , gadmdel_proc_date
                     );
                     
             

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5004_select_admdel: mysql_query error...\n");
		ZzLOG(ERROR, "daem5004_select_admdel: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5004_select_admdel: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5004_select_admdel: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 9;
	}

	memset(&gcont_gu  , 0x00, sizeof(gcont_gu  ));
	memset(&gid       , 0x00, sizeof(gid       ));

	if (row = mysql_fetch_row(res))
	{
		strcpy(gcont_gu  , getstr(row, 0));
		gid              = (unsigned long)getnum(row, 1);
	}
	mysql_free_result(res);
	return 0;
}


//******************************************************************************
//* daem5004_delete_data(°ø°³ÀÚ·á½Ç »èÁ¦Ã³¸®) -- °ü¸®ÀÚ »èÁ¦
//******************************************************************************
int daem5004_delete_data()
{
	char szQuery[5000];		// query string
	int  ret = 0;

	//--------------------------------------------------------------------------
	//  UPDATE Ã³¸®¿Ï·áÃ³¸®
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "UPDATE zangsi.T_CONTENTS_ADMDEL	"
	                 "   SET proc_yn   = 'Y'        	"
	                 " WHERE cont_gu =  '%s'        	"
	                 "   AND id      =  %ld   ; /*daem5004*/ "
	                 ,gcont_gu
	                 ,gid
	                 );
	if (mysql_query(con, szQuery))
	{
	    ZzLOG(ERROR, "daem5004_delete_data: UPDATE T_CONTENTS_ADMDEL error...\n");
		ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	if ( strcmp(gcont_gu, "01") == 0 || strcmp(gcont_gu, "03") == 0) //À§µð½ºÅ© ÀÚ·á½Ç ÀÏ°æ¿ì
	{ 
	    
		//----------------------------------------------------------------------
		// ¼ºÀÎÀÏ °æ¿ì ³»¿ª ³²±â±âÀ§ÇØ ·Î±× ±â·ÏÇÑ´Ù. 20100806. hcs
		//	ÀüÃ¼ ÄÁÅÙÃ÷¿¡ ´ëÇØ¼­ ³»¿ª ³²±â±â 20120516
		//----------------------------------------------------------------------
		//if(ret == 1)
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi.T_CONTENTS_INFO_DEL "
							 " (id , title , descript , descript2 , descript3 , keyword , sect_code , sect_sub  "
							 " , adult_yn , share_meth , price_amt , won_mega , disp_end_date , disp_end_time ,  "
							 " memo_cnt , memo_new , disp_stat , disp_cnt_inc , nmnt_cnt , file_del_yn , del_yn  "
							 " , reg_user , reg_date , reg_time , item_bold_yn , item_color , bomul_id ,  "
							 " bomul_stat , req_id , editor_type , copyright_yn , del_reg_user, del_reg_date , del_reg_time) "
							 " SELECT "
							 " a.id , a.title , a.descript , a.descript2 , a.descript3 , a.keyword ,  "
							 " a.sect_code , a.sect_sub , a.adult_yn , a.share_meth , a.price_amt , a.won_mega  "
							 " , a.disp_end_date , a.disp_end_time , a.memo_cnt , a.memo_new , a.disp_stat ,  "
							 " a.disp_cnt_inc , a.nmnt_cnt , a.file_del_yn , a.del_yn , a.reg_user , a.reg_date  "
							 " , a.reg_time , a.item_bold_yn , a.item_color , a.bomul_id , a.bomul_stat ,  "
							 " a.req_id , a.editor_type , ifnull(b.copyright_yn, 'N') , 'system', '%s' , date_format(now(), '%%H%%i%%s') "
							 " FROM zangsi.T_CONTENTS_INFO a left outer join zangsi.T_CONTENTS_VIR_ID b on a.id = b.id "
							 " where a.id = %lu ; /*daem5004*/ "
							 , "20170101"
							 , gid);
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5004_delete_data: insert into zangsi.T_CONTENTS_INFO_DEL error. [%d](%s)\n",mysql_errno(con), mysql_error(con));
			    ZzLOG(ERROR, "daem5004_delete_data: Query=(%s)\n", szQuery);
				if( mysql_errno(con) != 1062 )
					return -1;
		    }


			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi.T_CONTENTS_FILE_DEL "
							 " (id , folder_yn , server_id , file_path , file_name1 , file_name2 , file_size ,  "
							 " file_type , file_resoX , file_resoY , qury_cnt , down_cnt , fix_down_cnt ,  "
							 " up_st_date , up_st_time , explan_type , dsp_file_cnt , reg_user , reg_date ,  "
							 " reg_time) "
							 " SELECT "
							 " id , folder_yn , server_id , file_path , file_name1 , file_name2 , file_size ,  "
							 " file_type , file_resoX , file_resoY , qury_cnt , down_cnt , fix_down_cnt ,  "
							 " up_st_date , up_st_time , explan_type , dsp_file_cnt , reg_user , reg_date ,  "
							 " reg_time "
							 " FROM zangsi.T_CONTENTS_FILE "
							 " where id = %lu ; /*daem5004*/ " 
							 , gid);
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5004_delete_data: insert into zangsi.T_CONTENTS_FILE_DEL error. [%d](%s)\n",mysql_errno(con), mysql_error(con));
			    ZzLOG(ERROR, "daem5004_delete_data: Query=(%s)\n", szQuery);
				if( mysql_errno(con) != 1062 )
					return -1;
		    }
		}	
		//----------------------------------------------------------------------
		// ¸ð¹ÙÀÏ ÄÁÅÙÃ÷Á¤º¸ »èÁ¦ - 20130410
		//----------------------------------------------------------------------
		memset(szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " insert into zangsi.T_MOB_CONTENTS_INFO_DEL "
						 " (id , price_amt , file_size , reg_user , reg_date , reg_time , del_user , del_date ,del_time   )"
						 " SELECT "
						 " a.id , a.price_amt , a.file_size , a.reg_user , a.reg_date , a.reg_time  , 'system', '%s' , date_format(now(), '%%H%%i%%s')  "
						 " FROM zangsi.T_MOB_CONTENTS_INFO a "
						 " where a.id = %lu ; /*daem5004*/ "
						 , "20170101"
						 , gid);
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5004_delete_data: insert into zangsi.T_CONTENTS_INFO_DEL error. [%d](%s)\n",mysql_errno(con), mysql_error(con));
		    ZzLOG(ERROR, "daem5004_delete_data: Query=(%s)\n", szQuery);
		    if( mysql_errno(con) != 1062 )
				return -1;
	    }
		

/* 2ÆÀ¿¡´Â ÀÖÀ¸³ª 1ÆÀ¿¡´Â ¾ø´Â ºÎºÐ. ÂòÇÏ±â ¸ñ·ÏÀÌ¶ó°í ÇÑ´Ù. (Á¤Ãµ±â Â÷Àå) 2015.12.30
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_BASKET WHERE cont_id = %lu ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data DELETE T_CONTDATA_FILE error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }
*/

		usleep(300000);
		//----------------------------------------------------------------------
		// ÄÁÅÙÃ÷Á¤º¸ »èÁ¦
		//----------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_INFO WHERE id = %ld ; /*daem5004*/ ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data: DELETE T_CONTENTS_INFO error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }

		//--------------------------------------------------------------------------
		// ÆÄÀÏÁ¤º¸ »èÁ¦
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_FILE WHERE id = %ld ; /*daem5004*/ ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data DELETE T_CONTENTS_FILE error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }
		//--------------------------------------------------------------------------
		// ÄÁÅÙÃ÷ ´Ù¿î·Îµå Á¤º¸ »èÁ¦
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_FILE_USER_CNT WHERE id = %ld ; /*daem5004*/ ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data DELETE T_CONTENTS_FILE_USER_CNT error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }

		//--------------------------------------------------------------------------
		// ¸Þ¸ðÁ¤º¸ »èÁ¦
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_MEMO WHERE id = %ld ; /*daem5004*/ ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data: DELETE T_CONTENTS_MEMO error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }

		//--------------------------------------------------------------------------
		// ÆÄÀÏ¸®½ºÆ® »èÁ¦
		//--------------------------------------------------------------------------
		
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_FILELIST WHERE id = %ld ; /*daem5004*/ ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data: DELETE T_CONTENTS_FILELIST error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }
	    
		usleep(300000);

		//----------------------------------------------------------------------
		// °Ë»ö¿£ÁøÀÇ »öÀÎÁ¤º¸ »èÁ¦
		//----------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_CREATE "
		                "        (id, cont_gu, udt_cd) "
		                " VALUES (%ld, '01', 'D') ; /*daem5004*/ "
		                , gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data: INSERT T_CONTENTS_CREATE error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }
			    
		//----------------------------------------------------------------------
		// °¡»ó ¾ÆÀÌµð Å×ÀÌºí Á¤º¸ »èÁ¦
		//----------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " delete from zangsi.T_CONTENTS_VIR_ID where  id = %ld ; /*daem5004*/" , gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data: [ %s ] \n" , szQuery);
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }
	    ZzLOG(ALWAY, "daem5004_delete_VIR_ID: [ %s ] \n" , szQuery);

		//----------------------------------------------------------------------
		// °¡»ó ¾ÆÀÌµð Å×ÀÌºí2 Á¤º¸ »èÁ¦ - 20081118 HCS
		//----------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, " delete from zangsi.T_CONTENTS_VIR_ID2 where  id = %ld ; /*daem5004*/ " , gid);
		if (mysql_query(con, szQuery))
		{
		    ZzLOG(ERROR, "daem5004_delete_data: [ %s ] \n" , szQuery);
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }
		ZzLOG(ALWAY, "daem5004_delete_VIR_ID2: [ %s ] \n" , szQuery);
		//--------------------------------------------------------------------------
		// ÄÁÅÙÃ÷ ´Ù¿îÄ«¿îÆ® Á¤º¸ »èÁ¦ --2009/05/18 HCS.
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_CNT WHERE id = %ld ; /*daem5004*/ ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data DELETE T_CONTENTS_CNT error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }
		usleep(300000);
	    	    
	}
	else if ( strcmp(gcont_gu,"FD") == 0 )
	{
		//»èÁ¦ ³»¿ª¿¡ ´ëÇØ¼­ ±â·Ï ³²±â±â 20120516
		{
			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi.T_CONTFLOG_INFO_DEL "
							 " ( id,cate_no,cate_sno,reg_user,reg_seq,title,descript,adult_yn,share_meth,price_amt "
							 " ,won_mega,search_yn,pass,disp_end_date,disp_end_time,disp_stat,disp_cnt_inc,file_del_yn,del_yn,reg_date,reg_time "
							 " , copyright_yn , del_reg_user, del_reg_date , del_reg_time ) "
							 " SELECT "
							 " a.id,a.cate_no,a.cate_sno,a.reg_user,a.reg_seq,a.title,a.descript,a.adult_yn,a.share_meth,a.price_amt "
							 " ,a.won_mega,a.search_yn,a.pass,a.disp_end_date,a.disp_end_time,a.disp_stat,a.disp_cnt_inc,a.file_del_yn,a.del_yn,a.reg_date,a.reg_time "
							 " , ifnull(b.copyright_yn, 'N') , 'system', '%s' , date_format(now(), '%%H%%i%%s') "
							 " FROM zangsi.T_CONTFLOG_INFO a left outer join zangsi.T_CONTFLOG_VIR_ID b on a.id = b.id "
							 " where a.id = %lu ; /*daem5004*/ "
							 , "20170101"
							 , gid);
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5004_delete_data: insert into zangsi.T_CONTFLOG_INFO_DEL error. [%d](%s)\n",mysql_errno(con), mysql_error(con));
			    ZzLOG(ERROR, "daem5004_delete_data: Query=(%s)\n", szQuery);
				if( mysql_errno(con) != 1062 )
					return -1;
		    }


			memset(szQuery, 0x00, sizeof(szQuery));
			sprintf(szQuery, " insert into zangsi.T_CONTFLOG_FILE_DEL "
							 " ( id,folder_yn,server_id,file_path,file_name1,file_name2,file_size,file_type "
							 " ,file_resoX,file_resoY,qury_cnt,down_cnt,up_st_date,up_st_time,reg_user,reg_date,reg_time )"
							 " SELECT "
							 " id,folder_yn,server_id,file_path,file_name1,file_name2,file_size,file_type "
							 " ,file_resoX,file_resoY,qury_cnt,down_cnt,up_st_date,up_st_time,reg_user,reg_date,reg_time "
							 " FROM zangsi.T_CONTFLOG_FILE "
							 " where id = %lu ; /*daem5004*/ " 
							 , gid);
			if (mysql_query(con, szQuery))
			{
			    ZzLOG(ERROR, "daem5004_delete_data: insert into zangsi.T_CONTFLOG_FILE_DEL error. [%d](%s)\n",mysql_errno(con), mysql_error(con));
			    ZzLOG(ERROR, "daem5004_delete_data: Query=(%s)\n", szQuery);
				if( mysql_errno(con) != 1062 )
					return -1;
		    }
		}
		//----------------------------------------------------------------------
		// ÄÁÅÙÃ÷Á¤º¸ »èÁ¦
		//----------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTFLOG_INFO WHERE id = %ld ; /*daem5004*/ ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data: DELETE T_CONTFLOG_INFO error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }

		//--------------------------------------------------------------------------
		// ÆÄÀÏÁ¤º¸ »èÁ¦
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTFLOG_FILE WHERE id = %ld ; /*daem5004*/ ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data DELETE T_CONTFLOG_FILE error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }

		//--------------------------------------------------------------------------
		// ¸Þ¸ðÁ¤º¸ »èÁ¦
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTFLOG_FILELIST WHERE id = %ld ; /*daem5004*/ ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data: DELETE T_CONTFLOG_FILELIST error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }
		
		usleep(300000);
	}
	else
	{
		ZzLOG(ERROR, " gcont_gu ¾øÀ½ [ %s ] \n",gcont_gu);
		/*
		//----------------------------------------------------------------------
		// ÄÁÅÙÃ÷Á¤º¸ »èÁ¦
		//----------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_ASK WHERE id = %ld ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data: DELETE T_CONTENTS_ASK error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }

		//--------------------------------------------------------------------------
		// ¸Þ¸ðÁ¤º¸ »èÁ¦
		//--------------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_MEMO WHERE id = %ld ", gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data: DELETE T_CONTENTS_MEMO error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }

		//----------------------------------------------------------------------
		// °Ë»ö¿£ÁøÀÇ »öÀÎÁ¤º¸ »èÁ¦
		//----------------------------------------------------------------------
		memset (szQuery, 0x00, sizeof(szQuery));
		sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_CREATE "
		                "        (id, cont_gu, udt_cd) "
		                " VALUES (%ld, '02', 'D');"
		                , gid);
		if (mysql_query(con, szQuery)){
		    ZzLOG(ERROR, "daem5004_delete_data: INSERT T_CONTENTS_CREATE error...\n");
			ZzLOG(ERROR, "daem5004_delete_data: [%d](%s)\n",mysql_errno(con), mysql_error(con));
			return -1;
	    }
	    */
	}

	return 0;
}


//******************************************************************************
//* daem5004_delete_ask
//*-----------------------------------------------------------------------------
//* ¼³¸í : Ã³¸®ÀÏÀÚ -37ÀÇ T_CONTENTS_ASKÀÇ ÀÚ·á¸¦ select ÇÑ´Ù.
//******************************************************************************
int daem5004_delete_ask()
{
	char szQuery[1000];		// query string

	//--------------------------------------------------------------------------
	// µð½ºÅ© Á¾·áÀÏÀÚ°¡ Ã³¸®ÀÏÀÚ -4ÀÏ ÀÎ ÀÚ·áÁß 1°ÇÀ» selectÇÑ´Ù.
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "SELECT a.id                    "
	                 "  FROM zangsi.T_CONTENTS_ASK a "
	                 " WHERE a.reg_date <= '%s'      "
	                 " ORDER BY a.reg_date LIMIT 1  ; /*daem5004*/ "
                     , gdel_date2
                     );

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5004_delete_ask: mysql_query error...\n");
		ZzLOG(ERROR, "daem5004_delete_ask: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
	}
	if (!(res = mysql_store_result(con))) {
	    ZzLOG(ERROR, "daem5004_delete_ask: mysql_store_result error...\n");
		ZzLOG(ERROR, "daem5004_delete_ask: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
 	if (mysql_num_rows(res)==0)	{
		mysql_free_result(res);
		return 9;
	}
	memset(&gid       , 0x00, sizeof(gid       ));
	if (row = mysql_fetch_row(res))
	{
		gid = (unsigned long)getnum(row, 0);
	}
	mysql_free_result(res);

	//----------------------------------------------------------------------
	// ¿äÃ»ÀÚ·á »èÁ¦
	//----------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_ASK WHERE id = %ld ; /*daem5004*/ ", gid);
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5004_delete_ask: DELETE T_CONTENTS_ASK error...\n");
		ZzLOG(ERROR, "daem5004_delete_ask: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	//--------------------------------------------------------------------------
	// ¿äÃ»¸Þ¸ð »èÁ¦
	//--------------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "DELETE FROM zangsi.T_CONTENTS_MEMO WHERE id = %ld ; /*daem5004*/ ", gid);
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5004_delete_ask: DELETE T_CONTENTS_MEMO error...\n");
		ZzLOG(ERROR, "daem5004_delete_ask: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	//----------------------------------------------------------------------
	// °Ë»ö¿£ÁøÀÇ »öÀÎÁ¤º¸ »èÁ¦
	//----------------------------------------------------------------------
	memset (szQuery, 0x00, sizeof(szQuery));
	sprintf(szQuery, "INSERT INTO zangsi.T_CONTENTS_CREATE "
	                "        (id, cont_gu, udt_cd) "
	                " VALUES (%ld, '02', 'D') ; /*daem5004*/ "
	                , gid);
	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "daem5004_delete_ask: INSERT T_CONTENTS_CREATE error...\n");
		ZzLOG(ERROR, "daem5004_delete_ask: [%d](%s)\n",mysql_errno(con), mysql_error(con));
		return -1;
    }

	return 0;
}

/*****************************************************************************
* DB¿¡¼­ system Date¸¦ ¾ò´Â´Ù.
* (I) void
* (R) int : Á¤»ó(0)/¿À·ù(-1)
*****************************************************************************/
int daem5004_get_sysdate()
{
	char szQuery[1000];		// query string
	char sztemp [100];      // query temp

	if (strcmp(gproc_date, "00000000") == 0)
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "SELECT date_format(now(),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -4 DAY),'%Y%m%d')");
//		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -2 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add(now(), INTERVAL -27 DAY),'%Y%m%d') ; /*daem5004*/ ");
	}
	else
	{
		memset(szQuery, 0x00, sizeof(szQuery));
		strcpy(szQuery, "SELECT date_format('");
		strcat(szQuery, gproc_date);
		strcat(szQuery, "','%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add('");
		strcat(szQuery, gproc_date);
//		strcat(szQuery, "', INTERVAL -3 DAY),'%Y%m%d')");
		strcat(szQuery, "', INTERVAL -2 DAY),'%Y%m%d')");
		strcat(szQuery, "     , date_format(date_add('");
		strcat(szQuery, gproc_date);
		strcat(szQuery, "', INTERVAL -27 DAY),'%Y%m%d') ; /*daem5004*/ ");
	}

	if (mysql_query(con, szQuery)){
	    ZzLOG(ERROR, "sysdate: mysql_query error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
	
	if (!(res = mysql_store_result(con)))
	{
	    ZzLOG(ERROR, "sysdate: mysql_store_result error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		return -1;
	}
 	if (mysql_num_rows(res)==0)
 	{
	    ZzLOG(ERROR, "sysdate: mysql_num_rows error...\n");
		ZzLOG(ERROR, "[%d](%s)",mysql_errno(con), mysql_error(con));
		mysql_free_result(res);
		return -1;
	}
	row = mysql_fetch_row(res);
	memset(gproc_date, 0x00, sizeof(gproc_date));
	memset(gdel_date1, 0x00, sizeof(gdel_date1));
	memset(gdel_date2, 0x00, sizeof(gdel_date2));

	strcpy(gproc_date,   getstr(row, 0));
	strcpy(gdel_date1,   getstr(row, 1));
	strcpy(gdel_date2,   getstr(row, 2));
	
	mysql_free_result(res);

	return 0;
}


/*****************************************************************************
* ÇÁ·Î±×·¥ ½ÃÀÛ·çÆ¾
* Àü¿ªº¯¼ö ÃÊ±âÈ­ ¹× µ¥ÀÌÅ¸º£ÀÌ½º ¿¬°á
* (I) void
* (R) int : Á¤»ó(0)/¿À·ù(-1)
*****************************************************************************/
int daem5004_init_process(int argc, char **argv)
{
	char stemp[128];
	int ret=0;
    /*
    ** Àü¿ªº¯¼ö ÃÊ±âÈ­
    */
    #ifdef __DEBUG
    ZzInitGlobalVariable2("Dd_", "/logs/daemon"); 
    #else
    ZzInitGlobalVariable2("d_", "/logs/daemon"); 
    #endif

    ZzLOG(ALWAY, "[daem5004]*****************ÇÁ·Î±×·¥ ½ÃÀÛ*****************\n");  
    ZzLOG(ALWAY, "[daem5004] »èÁ¦µÈ ÄÁÅÙÃ÷ Á¤¸® ÀÛ¾÷\n");  

    // ÆÄ¶ó¹ÌÅÍ °ª ¼³Á¤ ¹× ÃÊ±âÈ­
    if (argc != 2){
    	goto arg_error;
    }

	//--------------------------------------------------------------------------
	// DB ¿¬°á
	//--------------------------------------------------------------------------
	if (!(con=db_connect("zangsi")))
	{
		ZzLOG(ERROR, "DB¿¡ Á¢¼ÓÇÏÁö ¸ø ÇÏ¿´½À´Ï´Ù...\n");
		db_disconnect(con);
	   	return(-1); 
	}

	if (!(con_bck=db_connect_backup("zangsi")))
	{
		ZzLOG(ERROR, "main bck DB¿¡ Á¢¼ÓÇÏÁö ¸ø ÇÏ¿´½À´Ï´Ù...\n");
		db_disconnect(con_bck);
	   	return(-1); 
	}
	

	/* ó¸®ÀÏÀÚ */
	memset(gproc_date, 0x00, sizeof(gproc_date));
	strcpy(gproc_date, argv[1]);
	ret=daem5004_get_sysdate();
	if (ret < 0){
		db_disconnect(con);
		return -1;
	}

    return (0);


arg_error:
    ZzLOG(ERROR, "usage : %s YYYYMMDD XXXXX\n", argv[0]);
    ZzLOG(ERROR, "        YYYYMMDD(ó¸®ÀÏÀÚ)-> '00000000'À¿æ¿ì systemÀÏÀ¿Îó¸®\n");

    ZzPRT(ERROR, "usage : %s YYYYMMDD XXXXX\n", argv[0]);
    ZzPRT(ERROR, "        YYYYMMDD(ó¸®ÀÏÀÚ)-> '00000000'À¿æ¿ì systemÀÏÀ¿Îó¸®\n");
    return -1;
}

/***************************************************************************
* Çw¿¿¥ ~·á·ç¿
* µ¥ÀÌÅ¿£À¿º ~·á ¹× ó¸®°á°ú¸¦ ·¿×ÆÄÀ¿¡ dÀÇ
* (I) void
* (R) int : d»ó(0)/¿7ù(-1)
****************************************************************************/
int daem5004_term_process()
{
    // DB close
	db_disconnect(con);	
    ZzLOG(ALWAY, "[daem5004]*****************Çw¿¿¥ ~·á*****************\n\n");

    return (0);
}

/*****************************************************************************
* Çw¿¿¥ ½ñ¿Î ó¸®
* (I) void
* (R) void
*****************************************************************************/
void  daem5004_signal(int nSignal)
{
    daem5004_term_process();
}

/*****************************************************************************
*  Çw¿¿¥ ¸ÞÀÎ 
*****************************************************************************/
int main(int argc, char **argv)
{                
	char    szTemp[1024];
	int     rc;
                 
	/*       
	** SIGNAL dÀÇ
	*/       
/*
	signal(SIGTERM, daem5004_signal);
	signal(SIGINT,  daem5004_signal);
	signal(SIGQUIT, daem5004_signal);
	signal(SIGKILL, daem5004_signal);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
*/
	if ( daem5004_init_process(argc, argv) == 0 ) {
		/* Çw¿¿¥ ¸ÞÀ¿ç¿ */
		rc = daem5004_main_process();
		/* Çw¿¿¥ ~·á·ç¿ */
	}
	daem5004_term_process();
	return(0);
}
/*****************************************************************************
*  End of file...
*****************************************************************************/


