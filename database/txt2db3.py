#coding=utf-8
import os
import sqlite3
from itertools import islice

def txt2Paras_Table(dataFile,dbFile):
    f = open(dataFile)
    for line in f:
        if line !='' and line != '\n' and line.find('##')<0:
            list = line.split("\t")
            tbname = list[0]  
            dataRow = []
            for i in range(1,len(list)):
                dataRow.append(list[i])
            conn = sqlite3.connect(dbFile)
            cur = conn.cursor()
            create_table_sql = '''CREATE TABLE IF NOT EXISTS %s (
                               Parameter TEXT,
                               Description TEXT,
                               Unit TEXT DEFAULT NULL,
                               Module_ID TEXT DEFAULT NULL,
                               Value FLOAT DEFAULT NULL,
                               Impact FLOAT DEFAULT NULL,
                               Change TEXT DEFAULT NULL,
                               Max FLOAT DEFAULT NULL,
                               Min FLOAT DEFAULT NULL,
                               Use TEXT DEFAULT NULL
                               )''' % tbname
            cur.execute(create_table_sql)
            load_sql = '''insert into %s values (?,?,?,?,?,?,?,?,?,?)''' % tbname
            cur.execute(load_sql,dataRow)
            conn.commit()
    f.close()
    cur.close()
    conn.close()
def txt2SoilLookup(dataFile,dbFile):
    filename = dataFile.split('/')[-1]
    tbname = os.path.splitext(filename)[0]
    f = open(dataFile)
    for line in islice(f,1,None):
        if line !='' and line != '\n' and line.find('##')<0:
            list = line.split("\t") 
            dataRow = []
            for i in range(0,len(list)):
                dataRow.append(list[i])
            conn = sqlite3.connect(dbFile)
            cur = conn.cursor()
            create_table_sql = '''CREATE TABLE IF NOT EXISTS %s (
                               SOILCODE INT,
                               SNAM TEXT,
                               KS REAL DEFAULT NULL,
                               POROSITY REAL DEFAULT NULL,
                               FC REAL DEFAULT NULL,
                               P_INDEX REAL DEFAULT NULL,
                               RM REAL DEFAULT NULL,
                               WP REAL DEFAULT NULL,
                               B_DENSITY REAL DEFAULT NULL,
                               SAND REAL DEFAULT NULL,
                               CLAY REAL DEFAULT NULL,
                               SILT REAL DEFAULT NULL,
                               USLE_K REAL DEFAULT NULL,
                               TEXTURE INT DEFAULT NULL,
                               HG INT DEFAULT NULL
                               )''' % tbname
            cur.execute(create_table_sql)
            load_sql = '''insert into %(table)s values (%(arg)s)''' % {'table':tbname,'arg':','.join(['?' for i in range (0,len(list))]),}
            cur.execute(load_sql,dataRow)
            conn.commit()
    f.close()
    cur.close()
    conn.close()
def txt2LanduseLookup(dataFile,dbFile):
    filename = dataFile.split('/')[-1]
    tbname = os.path.splitext(filename)[0]
    f = open(dataFile)
    for line in islice(f,1,None):
        if line !='' and line != '\n' and line.find('##')<0:
            list = line.split("\t") 
            dataRow = []
            for i in range(0,len(list)):
                dataRow.append(list[i])
            conn = sqlite3.connect(dbFile)
            cur = conn.cursor()
            create_table_sql = '''CREATE TABLE IF NOT EXISTS %s (
                               LANDUSE_ID INT,
                               CODE TEXT,
                               LANDUSE_NAME TEXT DEFAULT NULL,
                               CN2A REAL DEFAULT NULL,
                               CN2B REAL DEFAULT NULL,
                               CN2C REAL DEFAULT NULL,
                               CN2D REAL DEFAULT NULL,
                               ROOT_DEPTH REAL DEFAULT NULL,
                               MANNING REAL DEFAULT NULL,
                               I_MAX REAL DEFAULT NULL,
                               I_MIN REAL DEFAULT NULL,
                               SHC REAL DEFAULT NULL,
                               SOIL_T10 REAL DEFAULT NULL,
                               USLE_C REAL DEFAULT NULL,
                               PET_FR REAL DEFAULT NULL,
                               PRC_ST1 REAL DEFAULT NULL,
                               PRC_ST2 REAL DEFAULT NULL,
                               PRC_ST3 REAL DEFAULT NULL,
                               PRC_ST4 REAL DEFAULT NULL,
                               PRC_ST5 REAL DEFAULT NULL,
                               PRC_ST6 REAL DEFAULT NULL,
                               PRC_ST7 REAL DEFAULT NULL,
                               PRC_ST8 REAL DEFAULT NULL,
                               PRC_ST9 REAL DEFAULT NULL,
                               PRC_ST10 REAL DEFAULT NULL,
                               PRC_ST11 REAL DEFAULT NULL,
                               PRC_ST12 REAL DEFAULT NULL,
                               SC_ST1 REAL DEFAULT NULL,
                               SC_ST2 REAL DEFAULT NULL,
                               SC_ST3 REAL DEFAULT NULL,
                               SC_ST4 REAL DEFAULT NULL,
                               SC_ST5 REAL DEFAULT NULL,
                               SC_ST6 REAL DEFAULT NULL,
                               SC_ST7 REAL DEFAULT NULL,
                               SC_ST8 REAL DEFAULT NULL,
                               SC_ST9 REAL DEFAULT NULL,
                               SC_ST10 REAL DEFAULT NULL,
                               SC_ST11 REAL DEFAULT NULL,
                               SC_ST12 REAL DEFAULT NULL,
                               DSC_ST1 REAL DEFAULT NULL,
                               DSC_ST2 REAL DEFAULT NULL,
                               DSC_ST3 REAL DEFAULT NULL,
                               DSC_ST4 REAL DEFAULT NULL,
                               DSC_ST5 REAL DEFAULT NULL,
                               DSC_ST6 REAL DEFAULT NULL,
                               DSC_ST7 REAL DEFAULT NULL,
                               DSC_ST8 REAL DEFAULT NULL,
                               DSC_ST9 REAL DEFAULT NULL,
                               DSC_ST10 REAL DEFAULT NULL,
                               DSC_ST11 REAL DEFAULT NULL,
                               DSC_ST12 REAL DEFAULT NULL
                               )''' % tbname
            cur.execute(create_table_sql)
            load_sql = '''insert into %(table)s values (%(arg)s)''' % {'table':tbname,'arg':','.join(['?' for i in range (0,len(list))]),}
            cur.execute(load_sql,dataRow)
            conn.commit()
    f.close()
    cur.close()
    conn.close()
def txt2CropLookup(dataFile,dbFile):
    filename = dataFile.split('/')[-1]
    tbname = os.path.splitext(filename)[0]
    f = open(dataFile)
    for line in islice(f,1,None):
        if line !='' and line != '\n' and line.find('##')<0:
            list = line.split("\t") 
            dataRow = []
            for i in range(0,len(list)):
                dataRow.append(list[i])
            conn = sqlite3.connect(dbFile)
            cur = conn.cursor()
            create_table_sql = '''CREATE TABLE IF NOT EXISTS %s (
                               ICNUM INT Primary key,
                               CPNM TEXT,
                               DESCRIPTION TEXT DEFAULT NULL,
                               IDC INT DEFAULT NULL,
                               BIO_E REAL DEFAULT NULL,
                               HVSTI REAL DEFAULT NULL,
                               BLAI REAL DEFAULT NULL,
                               FRGRW1 REAL DEFAULT NULL,
                               LAIMX1 REAL DEFAULT NULL,
                               FRGRW2 REAL DEFAULT NULL,
                               LAIMX2 REAL DEFAULT NULL,
                               DLAI REAL DEFAULT NULL,
                               CHTMX REAL DEFAULT NULL,
                               RDMX REAL DEFAULT NULL,
                               T_OPT REAL DEFAULT NULL,
                               T_BASE REAL DEFAULT NULL,
                               CNYLD REAL DEFAULT NULL,
                               CPYLD rREAL DEFAULT NULL,
                               BN1 REAL DEFAULT NULL,
                               BN2 REAL DEFAULT NULL,
                               BN3 REAL DEFAULT NULL,
                               BP1 REAL DEFAULT NULL,
                               BP2 REAL DEFAULT NULL,
                               BP3 REAL DEFAULT NULL,
                               WSYF REAL DEFAULT NULL,
                               USLE_C REAL DEFAULT NULL,
                               GSI REAL DEFAULT NULL,
                               VPDFR REAL DEFAULT NULL,
                               FRGMAX REAL DEFAULT NULL,
                               WAVP REAL DEFAULT NULL,
                               CO2HI REAL DEFAULT NULL,
                               BIOEHI REAL DEFAULT NULL,
                               RSDCO_PL REAL DEFAULT NULL,
                               OV_N REAL DEFAULT NULL,
                               CN2A REAL DEFAULT NULL,
                               CN2B REAL DEFAULT NULL,
                               CN2C REAL DEFAULT NULL,
                               CN2D REAL DEFAULT NULL,
                               FERTFIELD REAL DEFAULT NULL,
                               ALAI_MIN REAL DEFAULT NULL,
                               BIO_LEAF REAL DEFAULT NULL,
                               PHU REAL DEFAULT NULL,
                               CNOP REAL DEFAULT NULL,
                               LAI_INIT REAL DEFAULT NULL,
                               BIO_INIT REAL DEFAULT NULL,
                               CURYR_INIT INT DEFAULT NULL
                               )''' % tbname
            cur.execute(create_table_sql)
            load_sql = '''insert into %(table)s values (%(arg)s)''' % {'table':tbname,'arg':','.join(['?' for i in range (0,len(list))]),}
            cur.execute(load_sql,dataRow)
            conn.commit()
    f.close()
    cur.close()
    conn.close()
if __name__ == '__main__':
    dataFile1 = "G:/SEIMS/database/model_param_ini.txt"
    dataFile2 = "G:/SEIMS/database/SoilLookup.txt"
    dataFile3 = "G:/SEIMS/database/LanduseLookup.txt"
    dataFile4 = "G:/SEIMS/database/CropLookup.txt"
    DATA_BASE_DIR = r'G:\SEIMS\model_data\model_dianbu_30m_longterm'
    dbFile = DATA_BASE_DIR + os.sep + "Parameter.db3"
    txt2Paras_Table(dataFile1,dbFile)
    txt2SoilLookup(dataFile2,dbFile)
    txt2LanduseLookup(dataFile3,dbFile)
    txt2CropLookup(dataFile4,dbFile)
    
