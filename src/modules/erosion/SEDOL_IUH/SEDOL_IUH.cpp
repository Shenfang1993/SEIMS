#include "SEDOL_IUH.h"
#include "MetadataInfo.h"
#include "ModelException.h"
#include "util.h"
#include <map>
#include <omp.h>
#include <iostream>
#include <cmath>

using namespace std;

SEDOL_IUH::SEDOL_IUH(void): m_TimeStep(-1), m_cellSize(-1), m_CellWidth(NODATA_VALUE), m_nsub(-1), m_subbasin(NULL),
	m_iuhCell(NULL), m_DETSplash(NULL), m_iuhCols(-1),m_cellSedKgCols(-1), m_sedimentch(NULL), m_DETOverland(NULL),
	m_ManningN(NULL), m_Slope(NULL), m_Sed_Flow(NULL), m_cellSedKg(NULL), m_eco1(NODATA_VALUE), m_eco2(NODATA_VALUE), 
	m_V(NULL), m_Vol(NULL), m_Ctrans(NULL), m_SedDep(NULL)
{
	m_sedtoCh_T = NODATA_VALUE;
}

SEDOL_IUH::~SEDOL_IUH(void)
{
	//// cleanup
	if (this->m_DETOverland!=NULL) Release1DArray(m_DETOverland);
	if (this->m_Sed_Flow != NULL) Release1DArray(m_Sed_Flow);
	if (this->m_sedimentch != NULL) Release1DArray(m_sedimentch);
	if (this->m_Ctrans != NULL) Release1DArray(m_Ctrans);
	if (this->m_SedDep != NULL) Release1DArray(m_SedDep);
	if (this->m_Vol != NULL) Release1DArray(m_Vol);
	if (this->m_V != NULL) Release1DArray(m_V);

	if (this->m_cellSedKg != NULL) Release2DArray(m_cellSize, m_cellSedKg);
}

bool SEDOL_IUH::CheckInputData(void)
{
	if (m_cellSize < 0)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter: m_cellSize has not been set.");
	}
	if (FloatEqual(m_CellWidth, NODATA_VALUE))
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter: m_CellWidth has not been set.");
	}
	if (m_TimeStep <= 0)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter: m_TimeStep has not been set.");
	}
	if (m_subbasin == NULL)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter: m_subbasin has not been set.");
	}
	if (m_ManningN == NULL)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter: m_ManningN has not been set.");
	}
	if (m_DETSplash == NULL)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter: m_DETSplash has not been set.");
	}
	if (m_WH == NULL)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter: water depth has not been set.");
	}
	if(this->m_Ccoe < 0)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","You have not set the calibration coefficient of overland erosion.");
	}
	if (m_eco1 < 0)
	{
		throw ModelException(MID_KINWAVSED_OL, "CheckInputData", "You have not set the calibration coefficient 1.");
	}
	if (m_eco2 < 0)
	{
		throw ModelException(MID_KINWAVSED_OL, "CheckInputData", "You have not set the calibration coefficient 2.");
	}
	if (this->m_USLE_K == NULL)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","You have not set the USLE K (Erosion factor).");
	}
	if (this->m_USLE_C == NULL)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter of USLE_C can not be NULL.");
	}
	if (m_Slope == NULL)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter: slope has not been set.");
	}
	if (m_iuhCell == NULL)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter: m_iuhCell has not been set.");
	}
	/*if (m_rs == NULL)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter: m_rs has not been set.");
		return false;
	}*/
	if (m_date < 0)
	{
		throw ModelException("SEDOL_IUH","CheckInputData","The parameter: m_date has not been set.");
	}

	return true;
}

void SEDOL_IUH::initalOutputs()
{
	if(this->m_cellSize <= 0 || this->m_subbasin == NULL)				
		throw ModelException("SEDOL_IUH","CheckInputData","The dimension of the input data can not be less than zero.");
	// allocate the output variables
	if (m_SedDep == NULL)
	{
		m_SedDep = new float[m_cellSize];
	}
	if (m_Vol == NULL)
	{
		m_Vol = new float[m_cellSize];
		m_V = new float[m_cellSize];
		m_Ctrans = new float[m_cellSize];
		for (int i = 0; i < m_cellSize; i++)
		{
			m_Ctrans[i] = 0.f;
			m_SedDep[i] = 0.f;
			m_Vol[i] = 0.f;
			m_V[i] = 0.f;
		}
	}
	if(m_nsub <= 0)
	{
		map<int,int> subs;
		for(int i=0;i<this->m_cellSize;i++)
		{
			subs[int(this->m_subbasin[i])] += 1;
		}
		this->m_nsub = subs.size();
	}

	//initial some variables
	if(this->m_sedimentch == NULL) 
		m_sedimentch = new float[m_nsub+1];
	if(m_cellSedKg == NULL)
	{
		m_cellSedKg = new float*[m_cellSize];
		m_Sed_Flow = new float[m_cellSize];
		m_DETOverland = new float[m_cellSize];
		//get m_cellSedKgCols, i.e. the maximum of second column of iuh add 1.
		//#pragma omp parallel for
		for(int i=0; i<m_cellSize; i++)
		{
			m_cellSedKgCols = max(int(m_iuhCell[i][1]), m_cellSedKgCols);
			m_cellSedKg[i] = new float[m_cellSedKgCols+1];
			for(int j=0; j<=m_cellSedKgCols; j++)
			{
				m_cellSedKg[i][j] = 0.f;
				//cout << m_cellSedKg[i][j];
			}
			m_Sed_Flow[i] = 0.f;
			m_DETOverland[i] = 0.f;
			//cout << endl;
		}		
	}
}

void SEDOL_IUH::OverlandSedDetachment(int i)  //i is the id of the cell in the grid map  
{
	float nb = 0.1f;    // Manning roughness coefficient for bare soil
	float css = m_ManningN[i]/nb;      //critical shear stress
	//using Ariathurai and Arulanandan equation to calculate overland flow detachment
	// correction for slope dx/DX, water spreads out over larger area
	float s = max(0.001f, m_Slope[i] / 100.f);
	float S0 = sin(atan(s));
	float waterdepth = m_WH[i] / 1000;   // mm convert to m
	float Df, waterden, g, shearStr; 
	waterden = 1000;
	g = 9.8f;
	shearStr = waterden * g * waterdepth * S0;
	float tt = shearStr / css - 1;
	float maxtt = max(0.f, tt);
	Df = m_Ccoe * maxtt;       // g/m2s
	//Df = m_Ccoe * m_USLE_C[i] * m_USLE_K[i] * pow(shearStr, 1.5f);
	float cellareas = (m_CellWidth/cos(atan(s))) * m_CellWidth;
	m_DETOverland[i] = Df /1000 * (m_TimeStep) * cellareas;        //kg
}

void SEDOL_IUH::CalcuFlowDetachment(int i)  //i is the id of the cell in the grid map  
{
	// correction for slope dx/DX, water spreads out over larger area
	float s = max(0.001f, m_Slope[i] / 100.f);
	float S0 = sin(atan(s));
	float waterdepth = m_WH[i] / 1000;   // mm convert to m
	//float waterdepth = m_Runoff[i] / 1000;   // mm convert to m

	//using Foster equation to calculate overland flow detachment
	float Df, waterden, g, shearStr;      //,q
	waterden = 1000;
	g = 9.8f;
	shearStr = waterden * g * waterdepth * S0;
	Df = m_Ccoe * m_USLE_C[i] * m_USLE_K[i] * pow(shearStr, 1.5f);
	/*q = m_Q[i];
	Df = m_Ccoe * m_USLE_C[i] * m_USLE_K[i] * q * S0;*/
	// convert kg/(m2*min) to kg/cell
	float cellareas = (m_CellWidth/cos(atan(s))) * m_CellWidth;
	m_DETOverland[i] = Df * (m_TimeStep/60) * cellareas;
}

void SEDOL_IUH::GetSedimentInFlow(int id)
{
	// get transport capacity
	float q, S0, K;
	q = m_V[id] * m_WH[id] * 60.f;   // m2/s -> m2/min
	float s = max(0.001f, m_Slope[id]);
	S0 = sin(atan(s));
	K = m_USLE_K[id];
	float threadhold = 0.046f;
	if (q <= 0.f)
	{
		m_Ctrans[id] = 0.f;
	}
	else
	{
		if (q < threadhold)
			m_Ctrans[id] = m_eco1 * K * S0 * sqrt(q);   // kg/(m*min) kg per unit width per minute
		else
			m_Ctrans[id] = m_eco2 * K * S0 * pow(q, 2.0f);
		// kg/(m*min)  -> convert to kg/m3
		m_Ctrans[id] = m_Ctrans[id] / q;
	}

	/// get sediment deposition
	CalcuFlowDetachment(id);
	//get sediment in flow
	float DF = 0.f;
	DF = m_DETSplash[id] + m_DETOverland[id];
	if (DF > 0.f)
	{
		int min = int(this->m_iuhCell[id][0]);
		int max = int(this->m_iuhCell[id][1]);
		int col = min + 2;
		for(int k = min; k <= max; k++)
		{
			this->m_cellSedKg[id][k] +=	DF * m_iuhCell[id][col];
			col++;
		}
	}
	if(m_cellSedKg[id][0] < 0)
		m_cellSedKg[id][0] = 0.f;

	// calculate water volume
	float slope, wh;
	slope = atan(m_Slope[id]);
	wh = m_WH[id] / 1000.f;  //mm -> m
	m_Vol[id] = m_CellWidth / cos(slope) * m_CellWidth * wh;  // m3

	//while the sediment in flow is more than the capacity of the overland flow,
	// then the sediment deposition would happen; if the velocity of flow is reduced, the TC is decreased correspondingly,
	// then, the sediment deposition may take place.
	float Deposition, concentration;
	if (m_Vol[id] > 0)
	{
		concentration = m_cellSedKg[id][0] / m_Vol[id];    // convert to kg/m3
	} else
	{
		concentration = 0.f;
	}
	Deposition = max(concentration - m_Ctrans[id], 0.f);   //kg/m3, >0
	if (Deposition > 0)
	{
		m_cellSedKg[id][0] = m_Ctrans[id] * m_Vol[id];
	}
	m_SedDep[id] = Deposition * m_Vol[id]; //kg/cell
}

int SEDOL_IUH::Execute()
{
	this->CheckInputData();

	this->initalOutputs();

//#pragma omp parallel for
	for (int n = 0; n<m_nsub+1; n++)
	{
		m_sedimentch[n] = 0.f;     // delete value of last time step
	}

	//int nt = 0;
	//float qs_cell = 0.f;
	float area = m_CellWidth * m_CellWidth;

	//#pragma omp parallel for
	for (int i = 0; i < m_cellSize; i++)
	{
		//forward one time step
		// added by Wu Hui, 2012.12.14
		int min = int(this->m_iuhCell[i][0]);
		int max = int(this->m_iuhCell[i][1]);
		for(int j=min; j<=max; j++)
		{
			if(j != max)	
			{
				//cout << j<<": "<<m_cellSedKg[i][j+1]<<"\n";
				this->m_cellSedKg[i][j] = this->m_cellSedKg[i][j+1];
			}
			else							
				this->m_cellSedKg[i][j] = 0.f;	
		}
		
		//add today's flow
		int subi = (int)m_subbasin[i];
		if (m_nsub == 1)
		{
			subi = 1;
		}
		else if(subi >= m_nsub+1) 
		{
			ostringstream oss;
			oss << subi;
			throw ModelException("SEDOL_IUH","Execute","The subbasin "+oss.str()+" is invalid.");
		}

		//CalcuFlowDetachment(i);
		GetSedimentInFlow(i);  //modified by Gao

		m_sedimentch[subi] += this->m_cellSedKg[i][0];	//get new value

		m_Sed_Flow[i] = m_cellSedKg[i][0];

		m_sedtoCh_T += m_cellSedKg[i][0];   // for BMP optimization

	}

	float tmp = 0.f;
	//#pragma omp parallel for reduction(+:tmp)
	for (int n = 1; n<m_nsub+1; n++)
	{
		tmp += m_sedimentch[n];	    //get overland flow routing for entire watershed. 
		//cout <<  n<<": "<< m_Sed_SOER[n] << endl;
	}
	m_sedimentch[0] = tmp;

	return 0;
}

bool SEDOL_IUH::CheckInputSize(const char* key, int n)
{
	if(n<=0)
	{
		throw ModelException("SEDOL_IUH","CheckInputSize","Input data for "+string(key) +" is invalid. The size could not be less than zero.");
		return false;
	}
	if(this->m_cellSize != n)
	{
		if(this->m_cellSize <=0) this->m_cellSize = n;
		else
		{
			throw ModelException("SEDOL_IUH","CheckInputSize","Input data for "+string(key) +" is invalid. All the input data should have same size.");
			return false;
		}
	}

	return true;
}

void SEDOL_IUH::SetValue(const char* key, float value)
{
	string sk(key);

	if (StringMatch(sk, "TimeStep"))
	{
		m_TimeStep =(int) value;    // /3600 time step unit is hour, second convert to hour
	}
	//else if (StringMatch(sk, "nCells"))
	//{
	//	m_cellSize =(int) value;
	//}
	else if (StringMatch(sk, "CellWidth"))
	{
		m_CellWidth = value;
	}
	else if (StringMatch(sk, "ThreadNum"))
	{
		omp_set_num_threads((int)value);
	}
	else if(StringMatch(sk,"ccoe"))		    this->m_Ccoe = value;
	else if (StringMatch(sk, VAR_OL_SED_ECO1)) m_eco1 = value;
	else if (StringMatch(sk, VAR_OL_SED_ECO2)) m_eco2 = value;
	else									throw ModelException("SEDOL_IUH","SetValue","Parameter " + sk 
		+ " does not exist in SEDOL_IUH method. Please contact the module developer.");
	
}

void SEDOL_IUH::GetValue(const char* key, float* value)
{
	string sk(key);
	if(StringMatch(sk,"SEDTOCH_T"))	
	{
		*value = m_sedtoCh_T;
		//cout << m_chStorage[iOutlet]<<endl;   //m_qsSub  m_chStorage
	}

}

void SEDOL_IUH::Set1DData(const char* key, int n, float* data)
{

	this->CheckInputSize(key,n);

	//set the value
	string sk(key);
	if (StringMatch(sk,"subbasin"))
	{
		m_subbasin = data;
	}
	else if (StringMatch(sk,"D_DETSplash"))
	{
		m_DETSplash = data;
	}
	else if (StringMatch(sk, "Slope"))
	{
		m_Slope = data;
	}
	else if(StringMatch(sk,"Manning"))		this->m_ManningN = data;
	else if(StringMatch(sk,"USLE_K"))		this->m_USLE_K = data;
	else if(StringMatch(sk,"USLE_C"))		this->m_USLE_C = data;
	else if (StringMatch(sk, "D_SURU"))
	{
		m_WH = data;
	}
	else									throw ModelException("SEDOL_IUH","SetValue","Parameter " + sk + 
		" does not exist in SEDOL_IUH method. Please contact the module developer.");

}

void SEDOL_IUH::Set2DData(const char* key, int nRows, int nCols, float** data)
{
	
	string sk(key);
	if (StringMatch(sk,"Ol_iuh"))
	{
		this->CheckInputSize("Ol_iuh",nRows);

		m_iuhCell = data;
		m_iuhCols = nCols;
	}
	else									throw ModelException("SEDOL_IUH","SetValue","Parameter " + sk + 
		" does not exist in SEDOL_IUH method. Please contact the module developer.");
}

void SEDOL_IUH::Get1DData(const char* key, int* n, float** data)
{
	string sk(key);
	if (StringMatch(sk, VAR_SED_FLOW))  
	{
		*data = this->m_Sed_Flow;
		*n = this->m_cellSize;
	}
	else if (StringMatch(sk, "DETOverland"))
	{
		*data = m_DETOverland;
		*n = m_cellSize;
	}
	else if (StringMatch(sk, "SEDTOCH"))
	{
		*data = m_sedimentch;
		*n = m_nsub;
	}
	else
	{
		throw ModelException("SEDOL_IUH","getResult","Result " + sk + " does not exist in SEDOL_IUH method. Please contact the module developer.");
	}
}


