/** 
*	@file
*	@version	1.0
*	@author    Wu Hui
*	@date	5-August-2013
*
*	@brief	IUH overland method to calculate overland sediment routing
*
*	Revision:	Zhiqiang YU	
*   Date:		2011-2-22
*	Description:
*	1.	Add parameter CellWidth.
*	2.	Delete parameter uhminCell and uhmaxCell because the parameter Ol_iuh
*		contains these information. The first and second column of Ol_iuh is 
*		min time and max time.
*	3.	The number of subbasins (m_nsub) should get from m_subbasin rather than
*		from main program. So does variable m_nCells.
*	4.	Add varaible m_iuhCols to store the number of columns of Ol_iuh. In the 
*		meantime, add one parameter nCols to function SetIUHCell.
*	5.	Add variable m_cellFlow to store the flow of each cell in each day between
*		min time and max time. Its number of columns equals to the maximum of second
*		column of Ol_iuh add 1.
*	6.  Add function initial to initailize some variables.
*	7.	Modify function Execute.
*/
#pragma once
#include <string>
#include <ctime>
#include "api.h"

using namespace std;
#include "SimulationModule.h"

class SEDOL_IUH:public SimulationModule
{
public:
	SEDOL_IUH(void);
	~SEDOL_IUH(void);
	virtual int Execute();
	virtual void SetValue(const char* key, float data);
	virtual void GetValue(const char* key, float *data);
	virtual void Set1DData(const char* key, int n, float* data);
	virtual void Set2DData(const char* key, int nRows, int nCols, float** data);
	virtual void Get1DData(const char* key, int* n, float** data);

	bool CheckInputSize(const char* key, int n);
	bool CheckInputData(void);

private:

	/// time step (hr)
	int  m_TimeStep;
	/// cell size of the grid (the validate cells of the whole basin)
	int  m_cellSize;
	/// cell width of the grid (m)
	float  m_CellWidth;
	/// the total number of subbasins
	int m_nsub;  
	/// subbasin grid ( subwatersheds ID)
	float* m_subbasin;				
	/// start time of IUH for each grid cell
	///float* m_uhminCell; 
	/// end time of IUH for each grid cell
	///float* m_uhmaxCell;  
	/// IUH of each grid cell (1/s)
	float** m_iuhCell;   
	/// the number of columns of Ol_iuh
	int		m_iuhCols;
	/// Manning's roughness [-]
	float* m_ManningN;
	/// water depth mm
	float* m_WH;
	/// sediment splash detachment from splash erosion module (kg)
	float* m_DETSplash; 
	/// calibration coefficient
	float m_Ccoe;
	/// slope
	float* m_Slope;
	/// crop management factor
	float* m_USLE_C;
	/// soil erodibility factor 
	float* m_USLE_K;

	//temparory
	float** m_cellSedKg;	
	int	    m_cellSedKgCols;

	//output
	float* m_Sed_SOER;
	/// sediment in overland flow to streams for each subbasin (kg)
	float* m_sedimentch;
	float* m_DETOverland;
	float m_sedtoCh_T;


	void initalOutputs();

	void OverlandSedDetachment(int id);   //not used

	void CalcuFlowDetachment(int id);

};

