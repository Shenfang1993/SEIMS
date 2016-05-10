/*!
 * \ingroup base
 * \file MongoUtil.cpp
 * \brief Implementation of utility functions of mongoDB
 * \author Junzhi Liu, LiangJun Zhu
 * \date May 2016
 *
 * 
 */
#pragma once
#include "MongoUtil.h"
#include <sstream>
#include "ModelException.h"
#include "utils.h"
#include "util.h"
#include <fstream>

using namespace std;

int		GetIntFromBSONITER(bson_iter_t *iter)
{
	bson_type_t vtype = bson_iter_type(iter);
	const bson_value_t *vv = bson_iter_value(iter);
	if (vv->value_type == BSON_TYPE_INT32)
	{
		return (int)vv->value.v_int32;
	}
	else if (vv->value_type == BSON_TYPE_INT64)
	{
		return (int)vv->value.v_int64;
	}
	else
	{
		throw ModelException("MongoDB Utility", "ReadFromMongoDB", "Failed in get INT value.\n");
		return -1;
	}
}

float	GetFloatFromBSONITER(bson_iter_t *iter)
{
	bson_type_t vtype = bson_iter_type(iter);
	const bson_value_t *vv = bson_iter_value(iter);
	if (vv->value_type == BSON_TYPE_INT32)
	{
		return (float)vv->value.v_int32;
	}
	else if (vv->value_type == BSON_TYPE_INT64)
	{
		return (float)vv->value.v_int64;
	}
	else if (vv->value_type == BSON_TYPE_DOUBLE)
	{
		return (float)vv->value.v_double;
	}
	else
	{
		throw ModelException("MongoDB Utility", "ReadFromMongoDB", "Failed in get FLOAT value.\n");
		return -1.f;
	}
}

time_t	GetDateTimeFromBSONITER(bson_iter_t *iter)
{
	bson_type_t vtype = bson_iter_type(iter);
	const bson_value_t *vv = bson_iter_value(iter);
	if (vv->value_type == BSON_TYPE_DATE_TIME)
	{
		return (time_t)vv->value.v_datetime;
	}
	else
	{
		throw ModelException("MongoDB Utility", "ReadFromMongoDB", "Failed in get Date Time value.\n");
		return -1;
	}
}

string	GetStringFromBSONITER(bson_iter_t *iter)
{
	bson_type_t vtype = bson_iter_type(iter);
	const bson_value_t *vv = bson_iter_value(iter);
	string vvStr = "";
	if (vv->value_type == BSON_TYPE_UTF8)
		vvStr = vv->value.v_utf8.str;
	else
		throw ModelException("MongoDB Utility", "ReadFromMongoDB", "Failed in get String value.\n");
	return vvStr;
}

int GetCollectionNames(mongoc_client_t* conn, string& dbName, vector<string>& collNameList)
{
	mongoc_database_t	*database;
	mongoc_cursor_t		*cursor;
	bson_error_t		*err = NULL;
	const bson_t		*doc;
	database = mongoc_client_get_database(conn,dbName.c_str());
	cursor = mongoc_database_find_collections(database,NULL,err);
	if (err != NULL)
	{
		throw ModelException("MongoUtil","GetCollectionNames","There is no collections in database: " + string(dbName) + ".\n");
	}
	bson_iter_t iter;
	while(mongoc_cursor_next(cursor,&doc)){
		if (bson_iter_init (&iter, doc) && bson_iter_find (&iter,"name")){
			string tmp = GetStringFromBSONITER(&iter);
			vector<string> tmpList = utils::SplitString(tmp,'.');
			vector<string>::iterator tmpIter = find(collNameList.begin(),collNameList.end(),tmpList[0]);
			if (tmpIter == collNameList.end())
				collNameList.push_back(tmpList[0]);
		}
	}
	vector<string>(collNameList).swap(collNameList);
	return 0;
}

void Read1DArrayFromMongoDB(mongoc_gridfs_t* spatialData, string& remoteFilename, clsRasterData* templateRaster, int& num, float*& data)
{
	mongoc_gridfs_file_t	*gfile;
	bson_t					*b;
	bson_error_t			*err = NULL;
	b = bson_new();
	BSON_APPEND_UTF8(b,"filename",remoteFilename.c_str());
	gfile = mongoc_gridfs_find_one(spatialData,b,err);
	if (err != NULL)
		throw ModelException("ModuleParamter","Read1DArrayFromMongoDB", "Failed in  gridfs_find_query filename: " + remoteFilename + "\n");
	size_t length = (size_t)mongoc_gridfs_file_get_length(gfile);
	mongoc_stream_t *stream = mongoc_stream_gridfs_new(gfile);
	num = length/4;
	data = new float[num];
	mongoc_iovec_t iov;
	iov.iov_base = (char*)data;
	iov.iov_len = length;
	ssize_t r = mongoc_stream_readv(stream,&iov,1,-1,0);
	if(templateRaster->getCellNumber() != num)
		throw ModelException("MongoUtil","Read1DArrayFromMongoDB","The data length in " + remoteFilename
		+ " is not the same as the template.");
	mongoc_stream_destroy(stream);
	mongoc_gridfs_file_destroy(gfile);
	bson_destroy(b);
}

void Read2DArrayFromMongoDB(mongoc_gridfs_t* spatialData, string& remoteFilename, clsRasterData* templateRaster, int& n, float**& data)
{
	mongoc_gridfs_file_t	*gfile;
	bson_t					*b;
	bson_error_t			*err = NULL;
	b = bson_new();
	BSON_APPEND_UTF8(b,"filename",remoteFilename.c_str());
	gfile = mongoc_gridfs_find_one(spatialData,b,err);
	if (err != NULL)
		throw ModelException("MongoUtil","Read2DArrayFromMongoDB", "Failed in  mongoc_gridfs_find_one filename: " + remoteFilename + "\n");
	size_t length = (size_t)mongoc_gridfs_file_get_length(gfile);
	mongoc_stream_t *stream = mongoc_stream_gridfs_new(gfile);
	float *floatValues = new float[length/4];
	mongoc_iovec_t iov;
	iov.iov_base = (char*)floatValues;
	iov.iov_len = length;
	ssize_t r = mongoc_stream_readv(stream,&iov,1,-1,0);
	int nRows = (int)floatValues[0];
	n = nRows;
	data = new float*[n];

	int index = 1;
	for(int i = 0; i < n; i++)
	{
		int nSub = int(floatValues[index]) + 1;
		data[i] = new float[nSub];

		data[i][0] = floatValues[index];
		for (int j = 1; j < nSub; j++)
			data[i][j] = floatValues[index+j];
		index = index + nSub;
	}
	mongoc_stream_destroy(stream);
	mongoc_gridfs_file_destroy(gfile);
	bson_destroy(b);
	free(floatValues);
}

void Read2DSoilAttr(mongoc_gridfs_t* spatialData, string remoteFileName, clsRasterData* templateRaster, int& n, float**& data)
{
	string remoteFile2 = remoteFileName + "2";
	clsRasterData raster1(spatialData, remoteFileName.c_str(), templateRaster);
	clsRasterData raster2(spatialData, remoteFile2.c_str(), templateRaster);

	float *data1, *data2;
	raster1.getRasterData(&n, &data1);
	raster2.getRasterData(&n, &data2);

	data = new float*[n];
	for(int i = 0; i < n; i++){
		data[i] = new float[2];
		data[i][0] = data1[i];
		data[i][1] = data2[i];
	}
}

void ReadIUHFromMongoDB(mongoc_gridfs_t* spatialData, string& remoteFilename, clsRasterData* templateRaster, int& n, float**& data)
{
	mongoc_gridfs_file_t	*gfile;
	bson_t					*b;
	bson_error_t			*err = NULL;
	b = bson_new();
	BSON_APPEND_UTF8(b,"filename",remoteFilename.c_str());
	gfile = mongoc_gridfs_find_one(spatialData,b,err);
	if (err != NULL)
		throw ModelException("MongoUtil","ReadIUHFromMongoDB", "Failed in  mongoc_gridfs_find_one filename: " + remoteFilename + "\n");
	size_t length = (size_t)mongoc_gridfs_file_get_length(gfile);
	mongoc_stream_t *stream = mongoc_stream_gridfs_new(gfile);
	float *floatValues = new float[length/4];
	mongoc_iovec_t iov;
	iov.iov_base = (char*)floatValues;
	iov.iov_len = length;
	ssize_t r = mongoc_stream_readv(stream,&iov,1,-1,0);

	n = (int)floatValues[0];
	data = new float*[n];

	int index = 1;
	for(int i = 0; i < n; i++)
	{
		int nSub = floatValues[index+1] - floatValues[index] + 3;
		data[i] = new float[nSub];

		data[i][0] = floatValues[index];
		data[i][1] = floatValues[index+1];
		for (int j = 2; j < nSub; j++)
			data[i][j] = floatValues[index+j];

		index = index + nSub;
	}
	mongoc_stream_destroy(stream);
	mongoc_gridfs_file_destroy(gfile);
	bson_destroy(b);
	free(floatValues);
}

void ReadLongTermMutltiReachInfo(mongoc_client_t *conn,string& dbName, int& nr, int& nc, float**& data)
{
	bson_t		*b = bson_new();
	bson_t		*child1 = bson_new();
	bson_t		*child2 = bson_new();
	BSON_APPEND_DOCUMENT_BEGIN(b,"$query",child1);
	bson_append_document_end(b,child1);
	BSON_APPEND_DOCUMENT_BEGIN(b,"$orderby",child2);
	BSON_APPEND_INT32(child2,REACH_SUBBASIN,1);
	bson_append_document_end(b,child2);
	bson_destroy(child1);
	bson_destroy(child2);

	mongoc_cursor_t		*cursor;
	const bson_t		*bsonTable;
	mongoc_collection_t	*collection;
	bson_error_t		*err = NULL;

	collection = mongoc_client_get_collection(conn,dbName.c_str(),DB_TAB_REACH);
	int nReaches = (int)mongoc_collection_count(collection,MONGOC_QUERY_NONE,b,0,0,NULL,err);
	if(err != NULL)
		throw ModelException("MongoUtil","ReadLongTermMutltiReachInfo","Failed to get document number of collection: " + string(DB_TAB_REACH) + ".\n");
	cursor = mongoc_collection_find(collection,MONGOC_QUERY_NONE,0,0,0,b,NULL,NULL);

	int nAttr = 10;
	float **tmpData = new float*[nReaches];
	for (int i = 0; i < nReaches; i++)
	{
		tmpData[i] = new float[nAttr];
	}
	bson_iter_t			itertor;
	int i = 0;
	while(mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor,&bsonTable)) 
	{
		if (bson_iter_init_find(&itertor,bsonTable,REACH_SUBBASIN))
			tmpData[i][0] = GetIntFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_DOWNSTREAM)) 
			tmpData[i][1] = GetIntFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_UPDOWN_ORDER)) 
			tmpData[i][2] = GetIntFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_WIDTH)) 
			tmpData[i][3] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_LENGTH)) 
			tmpData[i][4] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_DEPTH)) 
			tmpData[i][5] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_V0)) 
			tmpData[i][6] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_AREA)) 
			tmpData[i][7] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_MANNING)) 
			tmpData[i][8] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_SLOPE)) 
			tmpData[i][9] = GetFloatFromBSONITER(&itertor);
		i++;
	}

	data = new float*[nAttr];
	for (int i = 0; i < nAttr; i++)
		data[i] = new float[nReaches+1];

	for (int i = 0; i < nAttr; i++)
	{
		for (int j = 0; j < nReaches; j++)
		{
			data[i][j+1] = tmpData[j][i];// index of the reach is the ID in the reach table (1 to nReaches)
		}
	}

	nr = nAttr;
	nc = nReaches+1;

	for (int i = 0; i < nReaches; i++)
		delete[] tmpData[i];
	delete[] tmpData;
	bson_destroy(b);
	mongoc_collection_destroy(collection);
	mongoc_cursor_destroy(cursor);
}
void ReadLongTermReachInfo(mongoc_client_t *conn,string& dbName, int subbasinID, int& nr, int& nc, float**& data)
{
	bson_t		*b = bson_new();
	bson_t		*child1 = bson_new();
	BSON_APPEND_DOCUMENT_BEGIN(b,"$query",child1);
	BSON_APPEND_INT32(child1,REACH_SUBBASIN,1);
	BSON_APPEND_INT32(child1,REACH_GROUPDIVIDED,1);
	bson_append_document_end(b,child1);
	bson_destroy(child1);

	mongoc_cursor_t		*cursor;
	const bson_t		*bsonTable;
	mongoc_collection_t	*collection;
	bson_error_t		*err = NULL;

	collection = mongoc_client_get_collection(conn,dbName.c_str(),DB_TAB_REACH);
	if(err != NULL)
		throw ModelException("MongoUtil","ReadLongTermMutltiReachInfo","Failed to get document number of collection: " + string(DB_TAB_REACH) + ".\n");
	cursor = mongoc_collection_find(collection,MONGOC_QUERY_NONE,0,0,0,b,NULL,NULL);

	int nReaches = 1;
	int nAttr = 10;
	float **tmpData = new float*[nReaches];
	for (int i = 0; i < nReaches; i++)
	{
		tmpData[i] = new float[nAttr];
	}

	bson_iter_t			itertor;
	int i = 0;
	if(mongoc_cursor_next(cursor,&bsonTable)){
		if (bson_iter_init_find(&itertor,bsonTable,REACH_SUBBASIN))
			tmpData[i][0] = GetIntFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_DOWNSTREAM)) 
			tmpData[i][1] = GetIntFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_UPDOWN_ORDER)) 
			tmpData[i][2] = GetIntFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_WIDTH)) 
			tmpData[i][3] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_LENGTH)) 
			tmpData[i][4] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_DEPTH)) 
			tmpData[i][5] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_V0)) 
			tmpData[i][6] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_AREA)) 
			tmpData[i][7] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_MANNING)) 
			tmpData[i][8] = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_SLOPE)) 
			tmpData[i][9] = GetFloatFromBSONITER(&itertor);
	}
	else
		throw ModelException("MongoUtil","ReadLongTermReachInfo","Failed to query REACH information of SUBBASIN.\n");

	data = new float*[nAttr];
	for (int i = 0; i < nAttr; i++)
		data[i] = new float[nReaches+1];

	for (int i = 0; i < nAttr; i++)
	{
		for (int j = 0; j < nReaches; j++)
		{
			data[i][j+1] = tmpData[j][i];
		}
	}

	nr = nAttr;
	nc = nReaches+1;

	for (int i = 0; i < nReaches; i++)
		delete[] tmpData[i];
	delete[] tmpData;
	bson_destroy(b);
	mongoc_collection_destroy(collection);
	mongoc_cursor_destroy(cursor);
}

void ReadMutltiReachInfoFromMongoDB(LayeringMethod layeringMethod, mongoc_client_t *conn,string& dbName, int& nr, int& nc, float**& data)
{
	bson_t		*b = bson_new();
	bson_t		*child1 = bson_new();
	bson_t		*child2 = bson_new();
	BSON_APPEND_DOCUMENT_BEGIN(b,"$query",child1);
	bson_append_document_end(b,child1);
	BSON_APPEND_DOCUMENT_BEGIN(b,"$orderby",child2);
	BSON_APPEND_INT32(child2,REACH_SUBBASIN,1);
	bson_append_document_end(b,child2);
	bson_destroy(child1);
	bson_destroy(child2);

	mongoc_cursor_t		*cursor;
	const bson_t		*bsonTable;
	mongoc_collection_t	*collection;
	bson_error_t		*err = NULL;

	collection = mongoc_client_get_collection(conn,dbName.c_str(),DB_TAB_REACH);
	int nReaches = (int)mongoc_collection_count(collection,MONGOC_QUERY_NONE,b,0,0,NULL,err);
	if(err != NULL)
		throw ModelException("MongoUtil","ReadMutltiReachInfoFromMongoDB","Failed to get document number of collection: " + string(DB_TAB_REACH) + ".\n");
	cursor = mongoc_collection_find(collection,MONGOC_QUERY_NONE,0,0,0,b,NULL,NULL);
	vector< vector<float> > vecReaches;
	float id, upDownOrder, downUpOrder, downStreamID, manning, v0;

	bson_iter_t			itertor;
	int i = 0;
	while(mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor,&bsonTable)) 
	{
		if (bson_iter_init_find(&itertor,bsonTable,REACH_SUBBASIN))
			id = GetIntFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_DOWNUP_ORDER)) 
			downUpOrder = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_UPDOWN_ORDER)) 
			upDownOrder = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_DOWNSTREAM)) 
			downStreamID = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_MANNING)) 
			manning = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_V0)) 
			v0 = GetFloatFromBSONITER(&itertor);
		vector<float> vec;
		vec.resize(5);
		vec[0] = id;
		if (layeringMethod == UP_DOWN)
			vec[1] = upDownOrder;//order
		else
			vec[1] = downUpOrder;//downstream id
		vec[2] = downStreamID;
		vec[3] = manning;//manning's n
		vec[4] = v0;

		vecReaches.push_back(vec);
	}
	int numRec = vecReaches.size();
	int nAttr = 5;
	data = new float*[nAttr];
	for (int i = 0; i < nAttr; i++)
	{
		data[i] = new float[numRec];
	}
	for (int i = 0; i < numRec; ++i)
	{
		data[0][i] = vecReaches[i][0];
		data[1][i] = vecReaches[i][1];//order
		data[2][i] = vecReaches[i][2];
		data[3][i] = vecReaches[i][3];//manning's n
		data[4][i] = vecReaches[i][4];
	}
	nr = nAttr;
	nc = numRec;
	bson_destroy(b);
	mongoc_collection_destroy(collection);
	mongoc_cursor_destroy(cursor);
}


void ReadReachInfoFromMongoDB(LayeringMethod layeringMethod, mongoc_client_t *conn, string& dbName, int nSubbasin, int& nr, int& nc, float**& data)
{
	bson_t		*b = bson_new();
	bson_t		*child1 = bson_new();
	BSON_APPEND_DOCUMENT_BEGIN(b,"$query",child1);
	BSON_APPEND_INT32(child1,REACH_SUBBASIN,1);
	BSON_APPEND_INT32(child1,REACH_GROUPDIVIDED,1);
	bson_append_document_end(b,child1);
	bson_destroy(child1);

	mongoc_cursor_t		*cursor;
	const bson_t		*bsonTable;
	mongoc_collection_t	*collection;
	bson_error_t		*err = NULL;

	collection = mongoc_client_get_collection(conn,dbName.c_str(),DB_TAB_REACH);
	if(err != NULL)
		throw ModelException("MongoUtil","ReadReachInfoFromMongoDB","Failed to get document number of collection: " + string(DB_TAB_REACH) + ".\n");
	cursor = mongoc_collection_find(collection,MONGOC_QUERY_NONE,0,0,0,b,NULL,NULL);

	int nAttr = 5;
	int nReaches = 1;
	data = new float*[nAttr];
	for (int i = 0; i < nAttr; i++)
	{
		data[i] = new float[nReaches];
	}

	float id, upDownOrder, downUpOrder, downStreamID, manning, v0;
	bson_iter_t			itertor;
	int i = 0;
	if(mongoc_cursor_next(cursor,&bsonTable)) 
	{
		if (bson_iter_init_find(&itertor,bsonTable,REACH_SUBBASIN))
			id = GetIntFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_DOWNUP_ORDER)) 
			downUpOrder = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_UPDOWN_ORDER)) 
			upDownOrder = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_DOWNSTREAM)) 
			downStreamID = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_MANNING)) 
			manning = GetFloatFromBSONITER(&itertor);
		if (bson_iter_init_find(&itertor,bsonTable,REACH_V0)) 
			v0 = GetFloatFromBSONITER(&itertor);
	}
	for (int i = 0; i < nReaches; ++i)
	{
		data[0][i] = id;
		if (layeringMethod == UP_DOWN)
			data[1][i] = upDownOrder;//order
		else
			data[1][i] = downUpOrder;//downstream id
		data[2][i] = downStreamID;
		data[3][i] = manning;//manning's n
		data[4][i] = v0;
	}

	nr = nAttr;
	nc = nReaches;
	bson_destroy(b);
	mongoc_collection_destroy(collection);
	mongoc_cursor_destroy(cursor);
}
