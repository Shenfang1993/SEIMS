/*!
 * \brief Implementation of clsRasterData class
 *
 * 1. Using GDAL and MongoDB (currently, mongo-c-driver 1.3.5)
 * 2. Array1D and Array2D raster data are supported
 * \author Junzhi Liu, LiangJun Zhu
 * \version 2.0
 * \date Apr. 2011
 * \revised May. 2016
 * 
 */

#include "clsRasterData.h"
#include <fstream>
//#include <vector>
#include "utils.h"
//#include "util.h"
#include "ModelException.h"
#include <iomanip>
/// include GDAL
#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_string.h"
#include "ogr_spatialref.h"

clsRasterData::clsRasterData()
{
    m_rasterPositionData = NULL;
    m_rasterData = NULL;
    m_mask = NULL;
    m_is2DRaster = false;
    m_raster2DData = NULL;
}

clsRasterData::clsRasterData(string rstFileName)
{
    m_rasterPositionData = NULL;
    m_rasterData = NULL;
    m_mask = NULL;
    m_fileName = rstFileName;
    m_is2DRaster = false;
    m_raster2DData = NULL;
    if (GetUpper(GetSuffix(rstFileName)) == "ASC")
        ReadASCFile(rstFileName);
    else
        ReadFromGDAL(rstFileName);
}

clsRasterData::clsRasterData(mongoc_gridfs_t *gfs, const char *remoteFilename, clsRasterData *templateRaster)
{
    m_rasterPositionData = NULL;
    m_rasterData = NULL;
    m_mask = NULL;
    m_fileName = "";
    m_is2DRaster = false;
    m_raster2DData = NULL;
    ReadFromMongoDB(gfs, remoteFilename, templateRaster);
}

clsRasterData::clsRasterData(string ascFileName, clsRasterData *mask)
{
    m_rasterPositionData = NULL;
    m_rasterData = NULL;
    m_mask = mask;
    m_fileName = ascFileName;
    ReadASCFile(ascFileName, mask);
}

clsRasterData::~clsRasterData(void)
{
    if (m_rasterData != NULL) delete[] m_rasterData;

    if (m_rasterPositionData != NULL && m_mask == NULL)
    {
        for (int i = 0; i < m_nCells; ++i)
        {
            if (m_rasterPositionData[i] != NULL)
                delete[] m_rasterPositionData[i];

        }
        delete[] m_rasterPositionData;
    }
    if (m_raster2DData != NULL)
    {
        for (int i = 0; i < m_nCells; ++i)
        {
            if (m_raster2DData[i] != NULL)
                delete[] m_raster2DData[i];

        }
        delete[] m_raster2DData;
    }
}

float clsRasterData::getAverage()
{
    float temp = 0.0f;
    for (int i = 0; i < m_nCells; i++)
    {
        temp += m_rasterData[i];
    }
    return temp / m_nCells;
}

float clsRasterData::getAverage(int lyr)
{
    if (!m_is2DRaster && m_nLyrs == 1)
        return getAverage();
    else
    {
        if (lyr < m_nLyrs && m_raster2DData != NULL)
        {
            float temp = 0.0f;
            for (int i = 0; i < m_nCells; i++)
            {
                temp += m_raster2DData[i][lyr];
            }
            return temp / m_nCells;
        }
        else
            throw ModelException("clsRasterData", "getAverage",
                                 "The given layer number is exceed the maximum layers.\n");
    }
}

int clsRasterData::getPosition(int row, int col)
{
    if (m_rasterPositionData == NULL) return -1;

    for (int i = 0; i < m_nCells; i++)
    {
        if (row == m_rasterPositionData[i][0] && col == m_rasterPositionData[i][1]) return i;
    }

    return -1;
}

int clsRasterData::getPosition(float x, float y)
{
    float xllCenter = this->getXllCenter();
    float yllCenter = this->getYllCenter();
    float dx = this->getCellWidth();
    float dy = this->getCellWidth();
    int nRows = this->getRows();
    int nCols = this->getCols();

    float xmin = xllCenter - dx / 2.f;
    float xMax = xmin + dx * nCols;
    if (x > xMax || x < xllCenter) throw ModelException("Raster", "At", "The x coordinate is beyond the scale!");

    float ymin = yllCenter - dy / 2.f;
    float yMax = ymin + dy * nRows;
    if (y > yMax || y < yllCenter) throw ModelException("Raster", "At", "The y coordinate is beyond the scale!");

    int nRow = (int) ((yMax - y) / dy); //calculate from ymax
    int nCol = (int) ((x - xmin) / dx); //calculate from xmin
    return getPosition(nRow, nCol);
}

void clsRasterData::getRasterData(int *nRows, float **data)
{
    *nRows = m_nCells;
    *data = m_rasterData;
}

void clsRasterData::get2DRasterData(int *nRows, int *nCols, float ***data)
{
    *nRows = m_nCells;
    *nCols = m_nLyrs;
    *data = m_raster2DData;
}

map<string, float> *clsRasterData::getRasterHeader()
{
    if (m_mask != NULL) return m_mask->getRasterHeader();
    return &m_headers;
}

void clsRasterData::getRasterPositionData(int *nRows, float ***data)
{
    if (m_mask != NULL)
        m_mask->getRasterPositionData(nRows, data);
    else
    {
        *nRows = m_nCells;
        *data = m_rasterPositionData;
    }
}

float clsRasterData::getValue(int validCellIndex)
{
    if (m_rasterData == NULL)
        throw ModelException("Raster", "getValue", "Please first initialize the raster object.");
    if (m_nCells < validCellIndex)
        throw ModelException("Raster", "getValue",
                             "The index is too big! There are not so many valid cell in the raster.");
    return m_rasterData[validCellIndex];
}

float *clsRasterData::getValue(int validCellIndex, int *nLyrs)
{
    if (m_nCells < validCellIndex)
        throw ModelException("Raster", "getValue",
                             "The index is too big! There are not so many valid cell in the raster.");
    if (m_is2DRaster)
    {
        if (m_raster2DData == NULL)
            throw ModelException("Raster", "getValue", "Please first initialize the 2D raster object.");
        float *cellValues = new float[m_nLyrs];
        for (int i = 0; i < m_nLyrs; i++)
            cellValues[i] = m_raster2DData[validCellIndex][i];
        *nLyrs = m_nLyrs;
        return cellValues;
    }
    else
    {
        if (m_rasterData == NULL)
            throw ModelException("Raster", "getValue", "Please first initialize the raster object.");
        *nLyrs = 1;
        float *cellValues = new float[1];
        cellValues[0] = m_rasterData[validCellIndex];
        return cellValues;
    }
}

float clsRasterData::getValue(clsRasterData *templateRasterData, float *rasterData, int row, int col)
{
    if (templateRasterData == NULL || rasterData == NULL) return NODATA_VALUE;
    int position = templateRasterData->getPosition(row, col);
    if (position == -1) return NODATA_VALUE;
    return rasterData[position];
}

float *clsRasterData::getValue(clsRasterData *templateRasterData, float *rasterData, int row, int col, int *nLyrs)
{
    if (templateRasterData == NULL || rasterData == NULL) return NULL;
    int position = templateRasterData->getPosition(row, col);
    if (position == -1) return NULL;
    return getValue(position, nLyrs);
}

float clsRasterData::getValue(int row, int col)
{
    if (m_rasterData == NULL) return NODATA_VALUE;
    int validCellIndex = this->getPosition(row, col);
    if (validCellIndex == -1)
        return NODATA_VALUE;
    else
        return this->getValue(validCellIndex);
}

float *clsRasterData::getValue(int row, int col, int *nLyrs)
{
    int validCellIndex = this->getPosition(row, col);
    if (validCellIndex == -1)
    {
        *nLyrs = -1;
        return NULL;
    }
    else
    {
        return getValue(validCellIndex, nLyrs);
    }
}

void clsRasterData::outputASCFile(string &filename)
{
    if (m_is2DRaster)
        clsRasterData::outputASCFile(m_headers, m_nCells, m_rasterPositionData, m_raster2DData, filename);
    else
        clsRasterData::outputASCFile(m_headers, m_nCells, m_rasterPositionData, m_rasterData, filename);
}

void clsRasterData::outputASCFile(map<string, float> header, int nRows, float **position, float *value,
                                  string filename)
{
    //float noData = -9999.0f;/// removed to util.h

    ofstream rasterFile(filename.c_str());
    /// write header
    rasterFile << HEADER_RS_NCOLS << " " << header[HEADER_RS_NCOLS] << "\n";
    rasterFile << HEADER_RS_NROWS << " " << header[HEADER_RS_NROWS] << "\n";
    rasterFile << HEADER_RS_XLL << " " << header[HEADER_RS_XLL] << "\n";
    rasterFile << HEADER_RS_YLL << " " << header[HEADER_RS_YLL] << "\n";
    rasterFile << HEADER_RS_CELLSIZE << " " << header[HEADER_RS_CELLSIZE] << "\n";
    rasterFile << HEADER_RS_NODATA << " " << setprecision(6) << header[HEADER_RS_NODATA] << "\n";

    //write file
    int rows = int(header[HEADER_RS_NROWS]);
    int cols = int(header[HEADER_RS_NCOLS]);

    int index = 0;
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            if (index < nRows)
            {
                if (position[index][0] == i && position[index][1] == j)
                {
                    rasterFile << setprecision(6) << value[index] << " ";
                    index++;
                }
                else rasterFile << setprecision(6) << NODATA_VALUE << " ";
            }
            else rasterFile << setprecision(6) << NODATA_VALUE << " ";
        }
        rasterFile << "\n";
    }
    rasterFile.close();
}

void clsRasterData::outputASCFile(map<string, float> header, int nRows, float **position, float **value,
                                  string filename)
{
    string prePath = GetPathFromFullName(filename);
    string coreName = GetCoreFileName(filename);
    int nLyrs = (int)header[HEADER_RS_LAYERS];
    for (int lyr = 0; lyr < nLyrs; lyr++)
    {
        stringstream oss;
        oss << prePath << coreName << "_" << (lyr + 1) << ASCIIExtension;
        ofstream rasterFile(oss.str().c_str());
        /// write header
        rasterFile << HEADER_RS_NCOLS << " " << header[HEADER_RS_NCOLS] << "\n";
        rasterFile << HEADER_RS_NROWS << " " << header[HEADER_RS_NROWS] << "\n";
        rasterFile << HEADER_RS_XLL << " " << header[HEADER_RS_XLL] << "\n";
        rasterFile << HEADER_RS_YLL << " " << header[HEADER_RS_YLL] << "\n";
        rasterFile << HEADER_RS_CELLSIZE << " " << header[HEADER_RS_CELLSIZE] << "\n";
        rasterFile << HEADER_RS_NODATA << " " << setprecision(6) << header[HEADER_RS_NODATA] << "\n";

        //write file
        int rows = int(header[HEADER_RS_NROWS]);
        int cols = int(header[HEADER_RS_NCOLS]);

        int index = 0;
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                if (index < nRows)
                {
                    if (position[index][0] == i && position[index][1] == j)
                    {
                        rasterFile << setprecision(6) << value[index][lyr] << " ";
                        index++;
                    }
                    else rasterFile << setprecision(6) << NODATA_VALUE << " ";
                }
                else rasterFile << setprecision(6) << NODATA_VALUE << " ";
            }
            rasterFile << "\n";
        }
        rasterFile.close();
    }
}

void clsRasterData::outputASCFile(clsRasterData *templateRasterData, float *value, string filename)
{
    int nRows;
    float **position;
    templateRasterData->getRasterPositionData(&nRows, &position);
    clsRasterData::outputASCFile(*(templateRasterData->getRasterHeader()), nRows, position, value, filename);
}

void clsRasterData::outputASCFile(clsRasterData *templateRasterData, float **value, string filename)
{
    int nRows;
    float **position;
    templateRasterData->getRasterPositionData(&nRows, &position);
    clsRasterData::outputASCFile(*(templateRasterData->getRasterHeader()), nRows, position, value, filename);
}

void clsRasterData::outputGTiff(string filename)
{
    if (m_is2DRaster)
        clsRasterData::outputGTiff(m_headers, m_srs, m_nCells, m_rasterPositionData, m_raster2DData, filename);
    else
        clsRasterData::outputGTiff(m_headers, m_srs, m_nCells, m_rasterPositionData, m_rasterData, filename);
}

void clsRasterData::outputGTiff(map<string, float> header, string &srs, int nValidCells, float **position, float *value,
                                string filename)
{
    float noDataValue = header[HEADER_RS_NODATA];
    int nCols = (int) header[HEADER_RS_NCOLS];
    int nRows = (int) header[HEADER_RS_NROWS];
    float xll = header[HEADER_RS_XLL];
    float yll = header[HEADER_RS_YLL];
    float dx = header[HEADER_RS_CELLSIZE];
    int n = nRows * nCols;
    float *data = new float[n];

    int index = 0;
    for (int i = 0; i < nRows; ++i)
    {
        for (int j = 0; j < nCols; ++j)
        {
            if (index < nValidCells)
            {
                if (position[index][0] == i && position[index][1] == j)
                {
                    data[i * nCols + j] = value[index];
                    index++;
                }
                else
                    data[i * nCols + j] = noDataValue;
            }
            else
                data[i * nCols + j] = noDataValue;
        }
    }

    const char *pszFormat = "GTiff";
    GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

    char **papszOptions = poDriver->GetMetadata();
    GDALDataset *poDstDS = poDriver->Create(filename.c_str(), nCols, nRows, 1, GDT_Float32, papszOptions);

    /// Write the data to new file
    GDALRasterBand *poDstBand = poDstDS->GetRasterBand(1);
    poDstBand->RasterIO(GF_Write, 0, 0, nCols, nRows, data, nCols, nRows, GDT_Float32, 0, 0);
    poDstBand->SetNoDataValue(noDataValue);

    double geoTrans[6];
    geoTrans[0] = xll;
    geoTrans[1] = dx;
    geoTrans[2] = 0;
    geoTrans[3] = yll + nRows * dx;
    geoTrans[4] = 0;
    geoTrans[5] = -dx;
    poDstDS->SetGeoTransform(geoTrans);
    poDstDS->SetProjection(srs.c_str());
    //OGRSpatialReference srs;
    //srs.importFromWkt();
    //char *pSrsWkt = NULL;
    //srs.exportToWkt(&pSrsWkt);
    //poDstDS->SetProjection(pSrsWkt);
    //CPLFree(pSrsWkt);

    GDALClose(poDstDS);

    delete[] data;
}

void clsRasterData::outputGTiff(map<string, float> header, string &srs, int nValidCells, float **position,
                                float **value, string filename)
{
    string prePath = GetPathFromFullName(filename);
    string coreName = GetCoreFileName(filename);
    int nLyrs = (int) header[HEADER_RS_LAYERS];
    float noDataValue = header[HEADER_RS_NODATA];
    int nCols = (int) header[HEADER_RS_NCOLS];
    int nRows = (int) header[HEADER_RS_NROWS];
    float xll = header[HEADER_RS_XLL];
    float yll = header[HEADER_RS_YLL];
    float dx = header[HEADER_RS_CELLSIZE];
    int n = nRows * nCols;

    for (int lyr = 0; lyr < nLyrs; lyr++)
    {
        stringstream oss;
        oss << prePath << coreName << "_" << (lyr + 1) << GTiffExtension;
        float *data = new float[n];
        int index = 0;
        for (int i = 0; i < nRows; ++i)
        {
            for (int j = 0; j < nCols; ++j)
            {
                if (index < nValidCells)
                {
                    if (position[index][0] == i && position[index][1] == j)
                    {
                        data[i * nCols + j] = value[index][lyr];
                        index++;
                    }
                    else
                        data[i * nCols + j] = noDataValue;
                }
                else
                    data[i * nCols + j] = noDataValue;
            }
        }

        const char *pszFormat = "GTiff";
        GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);

        char **papszOptions = poDriver->GetMetadata();
        GDALDataset *poDstDS = poDriver->Create(oss.str().c_str(), nCols, nRows, 1, GDT_Float32, papszOptions);

        /// Write the data to new file
        GDALRasterBand *poDstBand = poDstDS->GetRasterBand(1);
        poDstBand->RasterIO(GF_Write, 0, 0, nCols, nRows, data, nCols, nRows, GDT_Float32, 0, 0);
        poDstBand->SetNoDataValue(noDataValue);

        double geoTrans[6];
        geoTrans[0] = xll;
        geoTrans[1] = dx;
        geoTrans[2] = 0;
        geoTrans[3] = yll + nRows * dx;
        geoTrans[4] = 0;
        geoTrans[5] = -dx;
        poDstDS->SetGeoTransform(geoTrans);
        poDstDS->SetProjection(srs.c_str());
        GDALClose(poDstDS);

        delete[] data;
    }
}

void clsRasterData::outputGTiff(clsRasterData *templateRasterData, float *value, string rasterName)
{
    int nRows;
    float **position;
    templateRasterData->getRasterPositionData(&nRows, &position);
    string srs(templateRasterData->getSRS());
    clsRasterData::outputGTiff(*(templateRasterData->getRasterHeader()), srs, nRows, position, value, rasterName);
}

void clsRasterData::outputGTiff(clsRasterData *templateRasterData, float **value, string rasterName)
{
    int nRows;
    float **position;
    templateRasterData->getRasterPositionData(&nRows, &position);
    string srs(templateRasterData->getSRS());
    clsRasterData::outputGTiff(*(templateRasterData->getRasterHeader()), srs, nRows, position, value, rasterName);
}

void clsRasterData::outputToMongoDB(string remoteFilename, mongoc_gridfs_t *gfs)
{
    if (m_is2DRaster && m_nLyrs > 0)
        clsRasterData::outputToMongoDB(m_headers, m_srs, m_nCells, m_rasterPositionData, m_raster2DData, m_nLyrs,
                                       remoteFilename, gfs);
    else
        clsRasterData::outputToMongoDB(m_headers, m_srs, m_nCells, m_rasterPositionData, m_rasterData, remoteFilename,
                                       gfs);
}

void clsRasterData::outputToMongoDB(map<string, float> header, string &srs, int nValid, float **position, float *value,
                                    string remoteFilename, mongoc_gridfs_t *gfs)
{
    //float noData = -9999.0f; /// removed to util.h #define
    /// Prepare binary data
    int rows = int(header[HEADER_RS_NROWS]);
    int cols = int(header[HEADER_RS_NCOLS]);
    float *data = new float[rows * cols];

    int index = 0;
    int dataIndex = 0;
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            dataIndex = i * cols + j;
            if (index < nValid)
            {
                if (position[index][0] == i && position[index][1] == j)
                {
                    data[dataIndex] = value[index];
                    index++;
                }
                else
                    data[dataIndex] = NODATA_VALUE;
            }
            else
                data[dataIndex] = NODATA_VALUE;
        }
    }
    size_t iID = remoteFilename.find_first_of('_');
    int subbasinID = atoi(remoteFilename.substr(0, iID).c_str());
    bson_t *p = (bson_t *) malloc(sizeof(bson_t));
    bson_init(p);
    BSON_APPEND_UTF8(p, MONG_GRIDFS_ID, remoteFilename.c_str());
    BSON_APPEND_INT32(p, MONG_GRIDFS_SUBBSN, subbasinID);
    BSON_APPEND_DOUBLE(p, HEADER_RS_NCOLS, cols);
    BSON_APPEND_DOUBLE(p, HEADER_RS_NROWS, rows);
    BSON_APPEND_DOUBLE(p, HEADER_RS_XLL, header[HEADER_RS_XLL]);
    BSON_APPEND_DOUBLE(p, HEADER_RS_YLL, header[HEADER_RS_YLL]);
    BSON_APPEND_DOUBLE(p, HEADER_RS_CELLSIZE, header[HEADER_RS_CELLSIZE]);
    BSON_APPEND_DOUBLE(p, HEADER_RS_NODATA, NODATA_VALUE);
    BSON_APPEND_UTF8(p, HEADER_RS_SRS, srs.c_str());
    mongoc_gridfs_file_t *gfile;
	mongoc_gridfs_file_opt_t gopt = {0};
    gopt.filename = remoteFilename.c_str();
    gopt.content_type = "float";
    gopt.metadata = p;
    gfile = mongoc_gridfs_create_file(gfs, &gopt);
    mongoc_iovec_t ovec;
    ovec.iov_base = (char *) data;
    ovec.iov_len = rows * cols * sizeof(float);
	//ssize_t r = mongoc_gridfs_file_writev(gfile, &ovec, 1, 0);
	mongoc_gridfs_file_writev(gfile, &ovec, 1, 0);
    mongoc_gridfs_file_save(gfile);
    mongoc_gridfs_file_destroy(gfile);
    bson_destroy(p);
    free(p);
    delete data;
}

void clsRasterData::outputToMongoDB(map<string, float> header, string &srs, int inValid, float **position,
                                    float **value, int lyrs, string remoteFilename, mongoc_gridfs_t *gfs)
{
    /// Prepare binary data
    int rows = int(header[HEADER_RS_NROWS]);
    int cols = int(header[HEADER_RS_NCOLS]);
    int nLyrs = lyrs;
    float *data = new float[rows * cols * nLyrs];

    int index = 0;
    int dataIndex = 0;
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            for (int k = 0; k < nLyrs; k++)
            {
                dataIndex = i * cols * nLyrs + j * nLyrs + k;
                if (index < inValid)
                {
                    if (position[index][0] == i && position[index][1] == j)
                    {
                        data[dataIndex] = value[index][k];
                        index++;
                    }
                    else
                        data[dataIndex] = NODATA_VALUE;
                }
                else
                    data[dataIndex] = NODATA_VALUE;
            }
        }
    }
    size_t iID = remoteFilename.find_first_of('_');
    int subbasinID = atoi(remoteFilename.substr(0, iID).c_str());
    bson_t *p = (bson_t *) malloc(sizeof(bson_t));
    bson_init(p);
    BSON_APPEND_UTF8(p, MONG_GRIDFS_ID, remoteFilename.c_str());
    BSON_APPEND_INT32(p, MONG_GRIDFS_SUBBSN, subbasinID);
    BSON_APPEND_DOUBLE(p, HEADER_RS_NCOLS, cols);
    BSON_APPEND_DOUBLE(p, HEADER_RS_NROWS, rows);
    BSON_APPEND_DOUBLE(p, HEADER_RS_XLL, header[HEADER_RS_XLL]);
    BSON_APPEND_DOUBLE(p, HEADER_RS_YLL, header[HEADER_RS_YLL]);
    BSON_APPEND_DOUBLE(p, HEADER_RS_CELLSIZE, header[HEADER_RS_CELLSIZE]);
    BSON_APPEND_DOUBLE(p, HEADER_RS_NODATA, NODATA_VALUE);
    BSON_APPEND_DOUBLE(p, HEADER_RS_LAYERS, nLyrs);
    BSON_APPEND_UTF8(p, HEADER_RS_SRS, srs.c_str());
    mongoc_gridfs_file_t *gfile;
    mongoc_gridfs_file_opt_t gopt = {0};
    gopt.filename = remoteFilename.c_str();
    gopt.content_type = "float";
    gopt.metadata = p;
    gfile = mongoc_gridfs_create_file(gfs, &gopt);
    mongoc_iovec_t ovec;
    ovec.iov_base = (char *) data;
    ovec.iov_len = rows * cols * nLyrs * sizeof(float);
    //ssize_t r = mongoc_gridfs_file_writev(gfile, &ovec, 1, 0);
	mongoc_gridfs_file_writev(gfile, &ovec, 1, 0);
    mongoc_gridfs_file_save(gfile);
    mongoc_gridfs_file_destroy(gfile);
    bson_destroy(p);
    free(p);
    delete data;
}

void clsRasterData::outputToMongoDB(clsRasterData *templateRasterData, float *value, string filename,
                                    mongoc_gridfs_t *gfs)
{
    int nRows;
    float **position;
    templateRasterData->getRasterPositionData(&nRows, &position);
    string srs(templateRasterData->getSRS());
    clsRasterData::outputToMongoDB(*(templateRasterData->getRasterHeader()), srs, nRows, position, value, filename,
                                   gfs);
}

void clsRasterData::outputToMongoDB(clsRasterData *templateRasterData, float **value, int lyrs, string filename,
                                    mongoc_gridfs_t *gfs)
{
    int nRows;
    float **position;
    templateRasterData->getRasterPositionData(&nRows, &position);
    string srs(templateRasterData->getSRS());
    clsRasterData::outputToMongoDB(*(templateRasterData->getRasterHeader()), srs, nRows, position, value, lyrs,
                                   filename, gfs);
}

void clsRasterData::outputWeightFile(clsRasterData *templateRasterData, int nCols, float weight, string filename)
{
    int nRows;
    float **position;
    templateRasterData->getRasterPositionData(&nRows, &position);

    ofstream rasterFile(filename.c_str());
    /// Write header
    rasterFile << nRows << "\n";
    rasterFile << nCols << "\n";

    /// Write file
    int rows = nRows;
    int cols = nCols;
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            rasterFile << weight << " ";
        }
        rasterFile << "\n";
    }
    rasterFile.close();
}

void clsRasterData::ReadASCFile(string ascFileName)
{
    utils util;
    if (!util.FileExists(ascFileName))
        throw ModelException("clsRasterData", "ReadASCFile",
                             "The file " + ascFileName + " does not exist or has not read permission.");
    //StatusMessage(("read " + ascFileName + "...").c_str());
    ifstream rasterFile(ascFileName.c_str());
    string tmp;
    float noData;
    int rows, cols;
    float tempFloat;
    vector<float> values;
    vector<int> positionRows;
    vector<int> positionCols;

    /// read header
    rasterFile >> tmp >> cols;
    m_headers[HEADER_RS_NCOLS] = float(cols);  ///m_headers[GetUpper(tmp)] = float(cols);
    rasterFile >> tmp >> rows;
    m_headers[HEADER_RS_NROWS] = float(rows);
    rasterFile >> tmp >> tempFloat;
    m_headers[HEADER_RS_XLL] = tempFloat;
    rasterFile >> tmp >> tempFloat;
    m_headers[HEADER_RS_YLL] = tempFloat;
    rasterFile >> tmp >> tempFloat;
    m_headers[HEADER_RS_CELLSIZE] = tempFloat;
    rasterFile >> tmp >> noData;
    m_headers[HEADER_RS_NODATA] = noData;

    /// get all valid values (i.e., exclude NODATA_VALUE)
    tempFloat = noData;
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            rasterFile >> tempFloat;
            if (tempFloat == noData) continue;
            values.push_back(tempFloat);
            positionRows.push_back(i);
            positionCols.push_back(j);
        }
    }
    rasterFile.close();

    /// create float array
    m_nCells = (int) values.size();
    m_rasterData = new float[m_nCells];
    m_rasterPositionData = new float *[m_nCells];
    for (int i = 0; i < m_nCells; ++i)
    {
        m_rasterData[i] = values.at(i);
        m_rasterPositionData[i] = new float[2];
        m_rasterPositionData[i][0] = float(positionRows.at(i));
        m_rasterPositionData[i][1] = float(positionCols.at(i));
    }
}

void clsRasterData::ReadASCFile(string ascFileName, clsRasterData *mask)
{
    if (mask == NULL) ReadASCFile(ascFileName);
    else
    {
        utils util;
        if (!util.FileExists(ascFileName))
            throw ModelException("clsRasterData", "ReadASCFile",
                                 "The file " + ascFileName +
                                 " does not exist or has not read permission.");
        ///StatusMessage(("read " + ascFileName + "...").c_str());
        /// Get the data position from mask
        int nRows;
        float **validPosition;
        mask->getRasterPositionData(&nRows, &validPosition);
        m_nCells = nRows;

        ifstream rasterFile(ascFileName.c_str());
        string tmp;
        float noDataValue;

        /// Read header and get header from mask
        rasterFile >> tmp >> tmp; /// cols
        rasterFile >> tmp >> tmp; /// rows
        rasterFile >> tmp >> tmp; /// xll
        rasterFile >> tmp >> tmp; /// yll
        rasterFile >> tmp >> tmp; /// cell size
        rasterFile >> tmp >> noDataValue; /// nodata
        /// Set header
        map<string, float> *maskHeader = mask->getRasterHeader();
        m_headers[HEADER_RS_NCOLS] = (*maskHeader)[HEADER_RS_NCOLS];
        m_headers[HEADER_RS_NROWS] = (*maskHeader)[HEADER_RS_NROWS];
        m_headers[HEADER_RS_NODATA] = (*maskHeader)[HEADER_RS_NODATA];
        m_headers[HEADER_RS_CELLSIZE] = (*maskHeader)[HEADER_RS_CELLSIZE];
        m_headers[HEADER_RS_XLL] = (*maskHeader)[HEADER_RS_XLL];
        m_headers[HEADER_RS_YLL] = (*maskHeader)[HEADER_RS_YLL];

        /// Read data
        m_rasterData = new float[nRows];
        int ascCols = mask->getCols();
        int ascRows = mask->getRows();

//        int index = 0;
        float *pData = new float[ascRows * ascCols];
        for (int i = 0; i < ascRows; ++i)
        {
            for (int j = 0; j < ascCols; ++j)
            {
                rasterFile >> pData[i * ascCols + j];
            }
        }
        /// Is this redundant? LJ
        for (int index = 0; index < nRows; index++)
        {
            int i = (int) validPosition[index][0];
            int j = (int) validPosition[index][1];
            int rasterIndex = i * ascCols + j;
            m_rasterData[index] = pData[rasterIndex];
        }
        delete pData;
        rasterFile.close();
    }
}

void clsRasterData::ReadFromGDAL(string filename)
{
    GDALDataset *poDataset = (GDALDataset *) GDALOpen(filename.c_str(), GA_ReadOnly);
    if (poDataset == NULL)
    {
        throw ModelException("clsRasterData", "ReadFromGDAL", "Open file " + filename + " failed.\n");
    }

    GDALRasterBand *poBand = poDataset->GetRasterBand(1);
    m_headers[HEADER_RS_NCOLS] = poBand->GetXSize();
    m_headers[HEADER_RS_NROWS] = poBand->GetYSize();
    m_headers[HEADER_RS_NODATA] = (float) poBand->GetNoDataValue();
    double adfGeoTransform[6];
    poDataset->GetGeoTransform(adfGeoTransform);
    m_headers[HEADER_RS_CELLSIZE] = (float) adfGeoTransform[1];
    m_headers[HEADER_RS_XLL] = (float) adfGeoTransform[0] + 0.5f * m_headers[HEADER_RS_CELLSIZE];
    m_headers[HEADER_RS_YLL] =
            (float) adfGeoTransform[3] + (m_headers[HEADER_RS_NROWS] - 0.5f) * m_headers[HEADER_RS_CELLSIZE];
    m_srs = string(poDataset->GetProjectionRef());
    int nRows = (int) m_headers[HEADER_RS_NROWS];
    int nCols = (int) m_headers[HEADER_RS_NCOLS];

    vector<float> values;
    vector<int> positionRows;
    vector<int> positionCols;
    /// Get all valid values
//    float tempFloat = m_headers[HEADER_RS_NODATA];

    GDALDataType dataType = poBand->GetRasterDataType();
    if (dataType == GDT_Float32)
    {
        float *pData = (float *) CPLMalloc(sizeof(float) * nCols * nRows);
        poBand->RasterIO(GF_Read, 0, 0, nCols, nRows, pData, nCols, nRows, GDT_Float32, 0, 0);
        for (int i = 0; i < nRows; ++i)
        {
            for (int j = 0; j < nCols; ++j)
            {
                int index = i * nCols + j;
                if (FloatEqual(pData[index], m_headers[HEADER_RS_NODATA]))
                    continue;
                values.push_back(pData[index]);
                positionRows.push_back(i);
                positionCols.push_back(j);
            }
        }
        CPLFree(pData);
    }
    else if (dataType == GDT_Int32)
    {
        int *pData = (int *) CPLMalloc(sizeof(int) * nCols * nRows);
        poBand->RasterIO(GF_Read, 0, 0, nCols, nRows, pData, nCols, nRows, GDT_Int32, 0, 0);
        for (int i = 0; i < nRows; ++i)
        {
            for (int j = 0; j < nCols; ++j)
            {
                int index = i * nCols + j;
                if (FloatEqual((float)pData[index], m_headers[HEADER_RS_NODATA]))
                    continue;
                values.push_back((float)pData[index]);
                positionRows.push_back(i);
                positionCols.push_back(j);
            }
        }
        CPLFree(pData);
    }
    else
    {
        throw ModelException("clsRasterData", "ReadFromGDAL",
                             "The data type of " + filename + " is neither GDT_Float32 nor GDT_Int32.");
    }

    GDALClose(poDataset);

    /// Create float array
    m_nCells = (int) values.size();
    m_rasterData = new float[m_nCells];
    m_rasterPositionData = new float *[m_nCells];
    for (int i = 0; i < m_nCells; ++i)
    {
        m_rasterData[i] = values.at(i);
        m_rasterPositionData[i] = new float[2];
        m_rasterPositionData[i][0] = float(positionRows.at(i));
        m_rasterPositionData[i][1] = float(positionCols.at(i));
    }
}

void clsRasterData::ReadFromGDAL(string fileName, clsRasterData *mask)
{
    if (mask == NULL)
        ReadFromGDAL(fileName);
    else
    {
        m_mask = mask;
        utils util;
        if (!util.FileExists(fileName))
            throw ModelException("clsRasterData", "ReadFromGDAL", "The file " + fileName +
                                                                  " does not exist or has not read permission.");
        StatusMessage(("read " + fileName + "...").c_str());
        /// Set header
        map<string, float> *maskHeader = mask->getRasterHeader();
        m_headers[HEADER_RS_NCOLS] = (*maskHeader)[HEADER_RS_NCOLS];
        m_headers[HEADER_RS_NROWS] = (*maskHeader)[HEADER_RS_NROWS];
        m_headers[HEADER_RS_NODATA] = (*maskHeader)[HEADER_RS_NODATA];
        m_headers[HEADER_RS_CELLSIZE] = (*maskHeader)[HEADER_RS_CELLSIZE];
        m_headers[HEADER_RS_XLL] = (*maskHeader)[HEADER_RS_XLL];
        m_headers[HEADER_RS_YLL] = (*maskHeader)[HEADER_RS_YLL];
        m_srs = string(mask->getSRS());
        /// Get the data position from mask
        int nRows;
        float **validPosition;
        mask->getRasterPositionData(&nRows, &validPosition);
        m_rasterPositionData = validPosition;
        m_nCells = nRows;

        /// Read data
        m_rasterData = new float[nRows];
        int ascCols = mask->getCols();
        int ascRows = mask->getRows();

        GDALDataset *poDataset = (GDALDataset *) GDALOpen(fileName.c_str(), GA_ReadOnly);
        if (poDataset == NULL)
        {
            throw ModelException("clsRasterData", "ReadFromGDAL", "Open file " + fileName + " failed.\n");
        }

        GDALRasterBand *poBand = poDataset->GetRasterBand(1);
        GDALDataType dataType = poBand->GetRasterDataType();

        if (dataType == GDT_Float32)
        {
            float *pData = (float *) CPLMalloc(sizeof(float) * ascCols * ascRows);
            poBand->RasterIO(GF_Read, 0, 0, ascCols, ascRows, pData, ascCols, ascRows, GDT_Float32, 0, 0);
            for (int index = 0; index < nRows; index++)
            {
                int i = (int) validPosition[index][0];
                int j = (int) validPosition[index][1];
                int rasterIndex = i * ascCols + j;
                m_rasterData[index] = pData[rasterIndex];
            }
            CPLFree(pData);
        }
        else if (dataType == GDT_Int32)
        {
            int *pData = (int *) CPLMalloc(sizeof(int) * ascCols * ascRows);
            poBand->RasterIO(GF_Read, 0, 0, ascCols, ascRows, pData, ascCols, ascRows, GDT_Int32, 0, 0);
            for (int index = 0; index < nRows; index++)
            {
                int i = (int) validPosition[index][0];
                int j = (int) validPosition[index][1];
                int rasterIndex = i * ascCols + j;
                m_rasterData[index] = (float)pData[rasterIndex];
            }
            CPLFree(pData);
        }
        else
        {
            throw ModelException("clsRasterData", "ReadFromGDAL",
                                 "The data type of " + fileName + " is neither GDT_Float32 nor GDT_Int32.");
        }

        GDALClose(poDataset);
    }
}

int clsRasterData::ReadFromMongoDB(mongoc_gridfs_t *gfs, const char *remoteFilename)
{
    /// Integrate 1D and 2D raster data
    mongoc_gridfs_file_t *gfile;
    bson_error_t *err = NULL;
    gfile = mongoc_gridfs_find_one_by_filename(gfs, remoteFilename, err);
    if (err != NULL)
    {
        throw ModelException("clsRasterData", "ReadRasterFromMongoDB",
                             "The file " + string(remoteFilename) + " does not exist.");
    }
    size_t length = (size_t) mongoc_gridfs_file_get_length(gfile);
    char *buf = (char *) malloc(length);
    mongoc_iovec_t iov;
    iov.iov_base = buf;
    iov.iov_len = length;
    mongoc_stream_t *stream;
    stream = mongoc_stream_gridfs_new(gfile);
	//ssize_t r = mongoc_stream_readv(stream, &iov, 1, -1, 0);
	mongoc_stream_readv(stream, &iov, 1, -1, 0);
    float *data = (float *) buf;
    /// Get metadata
    const bson_t *bmeta;
    bmeta = mongoc_gridfs_file_get_metadata(gfile);
    /// Get value of given keys
    bson_iter_t iter;
    const char *headers[7] = {HEADER_RS_NCOLS, HEADER_RS_NROWS, HEADER_RS_XLL, HEADER_RS_YLL, HEADER_RS_CELLSIZE,
                              HEADER_RS_NODATA, HEADER_RS_LAYERS};
    for (int i = 0; i < 7; i++)
    {
        if (bson_iter_init(&iter, bmeta) && bson_iter_find(&iter, headers[i]))
        {
            m_headers[string(bson_iter_key(&iter))] = GetFloatFromBSONITER(&iter);
        }
        else
            throw ModelException("clsRasterData", "ReadRasterFromMongoDB",
                                 "Failed in get FLOAT value: " + string(headers[i]) + "\n");
    }
    if (bson_iter_init(&iter, bmeta) && bson_iter_find(&iter, HEADER_RS_SRS))
    {
        m_srs = GetStringFromBSONITER(&iter);
    }
    else
        throw ModelException("clsRasterData", "ReadRasterFromMongoDB", "Failed in get SRS String value.\n");
    int nRows = (int) m_headers[HEADER_RS_NROWS];
    int nCols = (int) m_headers[HEADER_RS_NCOLS];
    m_nLyrs = (int) m_headers[HEADER_RS_LAYERS];

    /// Is 1D or 2D raster?
    vector<int> positionRows;
    vector<int> positionCols;
    float nodataFloat = m_headers[HEADER_RS_NODATA];
    if (m_nLyrs == 1)
    {
        m_is2DRaster = false;
        vector<float> values;
        //get all valid values
        for (int i = 0; i < nRows; ++i)
        {
            for (int j = 0; j < nCols; ++j)
            {
                int index = i * nCols + j;
                float value = data[index];
                if (FloatEqual(nodataFloat, value))
                    continue;
                values.push_back(value);
                positionRows.push_back(i);
                positionCols.push_back(j);
            }
        }

        //create float array
        m_nCells = values.size();
        m_rasterData = new float[m_nCells];
        m_rasterPositionData = new float *[m_nCells];
        for (int i = 0; i < m_nCells; ++i)
        {
            m_rasterData[i] = values.at(i);
            m_rasterPositionData[i] = new float[2];
            m_rasterPositionData[i][0] = float(positionRows.at(i));
            m_rasterPositionData[i][1] = float(positionCols.at(i));
        }
    }
    else
    {
        m_is2DRaster = true;
        vector<vector<float>> values(m_nLyrs);
        //get all valid values
        bool hasAddRowColPos = false;
        for (int i = 0; i < nRows; ++i)
        {
            for (int j = 0; j < nCols; ++j)
            {
                hasAddRowColPos = false;
                for (int k = 0; k < m_nLyrs; k++)
                {
                    int index = i * nCols * m_nLyrs + j * m_nLyrs + k;
                    float value = data[index];
                    if (FloatEqual(nodataFloat, value))
                        continue;
                    values[k].push_back(value);
                    if (!hasAddRowColPos)
                    {
                        positionRows.push_back(i);
                        positionCols.push_back(j);
                        hasAddRowColPos = true;
                    }
                }
            }
        }

        //create float array
        m_nCells = values[0].size();
        m_rasterPositionData = new float *[m_nCells];
        m_raster2DData = new float *[m_nCells];

        for (int i = 0; i < m_nCells; ++i)
        {
            m_raster2DData[i] = new float[m_nLyrs];
            for (int j = 0; j < m_nLyrs; i++)
            {
                m_raster2DData[i][j] = values[j].at(i);
            }
            m_rasterPositionData[i] = new float[2];
            m_rasterPositionData[i][0] = float(positionRows.at(i));
            m_rasterPositionData[i][1] = float(positionCols.at(i));
        }
    }
    free(buf);
    mongoc_gridfs_file_destroy(gfile);
    return 0;
}

int clsRasterData::ReadFromMongoDB(mongoc_gridfs_t *gfs, const char *remoteFilename, clsRasterData *mask)
{
    if (mask == NULL) ReadFromMongoDB(gfs, remoteFilename);
    else
    {
        mongoc_gridfs_file_t *gfile;
        bson_error_t *err = NULL;
        gfile = mongoc_gridfs_find_one_by_filename(gfs, remoteFilename, err);
        if (err != NULL || gfile == NULL)
        {
            throw ModelException("clsRasterData", "ReadRasterFromMongoDB",
                                 "The file " + string(remoteFilename) + " does not exist.");
        }
		//cout<<"Read "<<remoteFilename<<endl;
        /// Get metadata
        const bson_t *bmeta;
        bmeta = mongoc_gridfs_file_get_metadata(gfile);
        /// Get value of given keys
        bson_iter_t iter;
        if (bson_iter_init(&iter, bmeta) && bson_iter_find(&iter, HEADER_RS_LAYERS)) 
            m_nLyrs = GetIntFromBSONITER(&iter);
        else
            throw ModelException("clsRasterData", "ReadRasterFromMongoDB", "Failed in get FLOAT value: LAYERS.\n");
        size_t length = (size_t) mongoc_gridfs_file_get_length(gfile);
        char *buf = (char *) malloc(length);
        mongoc_iovec_t iov;
        iov.iov_base = buf;
        iov.iov_len = length;
        mongoc_stream_t *stream;
        stream = mongoc_stream_gridfs_new(gfile);
		//ssize_t r = mongoc_stream_readv(stream, &iov, 1, -1, 0);
		mongoc_stream_readv(stream, &iov, 1, -1, 0);
        float *data = (float *) buf;
        /// Get the valid raster data positions from mask
        int nRows;
        float **validPosition;
        mask->getRasterPositionData(&nRows, &validPosition);
        m_rasterPositionData = validPosition;
        m_nCells = nRows;
        m_mask = mask;
        /// Set header
        m_headers[HEADER_RS_NCOLS] = mask->getCols();
        m_headers[HEADER_RS_NROWS] = mask->getRows();
        m_headers[HEADER_RS_NODATA] = mask->getNoDataValue();
        m_headers[HEADER_RS_CELLSIZE] = mask->getCellWidth();
        m_headers[HEADER_RS_XLL] = mask->getXllCenter();
        m_headers[HEADER_RS_YLL] = mask->getYllCenter();
        m_headers[HEADER_RS_LAYERS] = m_nLyrs;
        m_srs = string(mask->getSRS());
        int ascCols = mask->getCols();
//        int ascRows = mask->getRows();
        /// Is 1D or 2D raster?
        if (m_nLyrs == 1)
        {
            m_is2DRaster = false;
            /// Read data
            m_rasterData = new float[nRows];
            for (int index = 0; index < nRows; index++)
            {
                int i = (int)validPosition[index][0];
                int j = (int)validPosition[index][1];
                int rasterIndex = i * ascCols + j;
                m_rasterData[index] = data[rasterIndex];
            }

        }
        else
        {
            m_is2DRaster = true;
            /// Read data
            m_raster2DData = new float *[m_nCells];
            for (int index = 0; index < m_nCells; ++index)
            {
                int i = (int)validPosition[index][0];
                int j = (int)validPosition[index][1];
                m_raster2DData[index] = new float[m_nLyrs];
                for (int k = 0; k < m_nLyrs; k++)
                {
                    int rasterIndex = i * ascCols * m_nLyrs + j * m_nLyrs + k;
                    m_raster2DData[index][k] = data[rasterIndex];
                }
            }
        }
        mongoc_gridfs_file_destroy(gfile);
        free(buf);
    }
    return 0;
}


//int clsRasterData::getCols()
//{
//	map<string,float>* header =  getRasterHeader();
//	return int((*header)["NCOLS"]);
//}


//void clsRasterData::getHeaders(string databasePath,map<string,float>* headers)
//{
//	DBManager dbman;
//	string sql;
//	slTable* tbl;
//	// open the database
//	dbman.Open(databasePath + File_ParameterDB);
//	// if there is not an error
//	if(dbman.IsError()) throw ModelException("clsRasterData","getHeaders","Can't open parameter database!");
//	// construct the SQL statement for the query
//	sql = "SELECT Parameter,Value FROM Header";
//	// run the query
//	tbl = dbman.Load(sql);
//	if (tbl->nRows == 0) throw ModelException("ModuleParamter","getParameterFromDatabase","Can't find ASC Headers in parameter database!");
//	headers->clear();
//	//headers = new map<string,float>();
//	for(int i=1;i<=tbl->nRows;i++)
//	{
//		(*headers)[tbl->FieldValue(i, 1)] = float(atof(tbl->FieldValue(i, 2).c_str()));
//	}	
//	delete tbl;
//	tbl = NULL;
//	dbman.Close();
//}

//int clsRasterData::getCellWidth()
//{
//	map<string,float>* header =  getRasterHeader();
//	return int((*header)[Tag_CellSize]);
//}

//int clsRasterData::getRows()
//{
//	map<string,float>* header =  getRasterHeader();
//	return int((*header)["NROWS"]);
//}