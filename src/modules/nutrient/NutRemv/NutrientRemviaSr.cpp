/*!
 * \file NutrientRemviaSr.cpp
 * \ingroup NutRemv
 * \author Huiran Gao
 * \date May 2016
 */

#include <iostream>
#include "NutrientRemviaSr.h"
#include "MetadataInfo.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include "ModelException.h"
#include "util.h"
#include <omp.h>
using namespace std;

NutrientRemviaSr::NutrientRemviaSr(void):
//input 
m_nCells(-1), m_cellWidth(-1), m_nSolLyrs(3), m_anion_excl(NULL), m_isep_opt(NULL), m_ldrain(NULL), m_surfr(NULL), m_nperco(NULL), m_flat(NULL),
	m_sol_perco(NULL), m_sol_wsatur(NULL), m_phoskd(NULL), m_pperco(NULL), m_sol_bd(NULL), m_conv_wt(NULL),
	//output 
	m_latno3(NULL), m_percn(NULL), m_surqno3(NULL), m_sol_no3(NULL), m_surqsolp(NULL), m_wshd_plch(NULL), m_sol_solp(NULL)
{

}

NutrientRemviaSr::~NutrientRemviaSr(void) {
	///TODO
}
bool NutrientRemviaSr::CheckInputSize(const char* key, int n) {
	if(n <= 0) {
		//StatusMsg("Input data for "+string(key) +" is invalid. The size could not be less than zero.");
		return false;
	}
	if(m_nCells != n) {
		if(m_nCells <=0) {
			m_nCells = n;
		} else {
			//StatusMsg("Input data for "+string(key) +" is invalid. All the input data should have same size.");
			ostringstream oss;
			oss << "Input data for "+string(key) << " is invalid with size: " << n << ". The origin size is " << m_nCells << ".\n";  
			throw ModelException("Denitrification","CheckInputSize",oss.str());
		}
	}
	return true;
}
bool NutrientRemviaSr::CheckInputData() {
	if(this->m_nCells == -1) {
		throw ModelException("NutRemv","CheckInputData","You have not set the date time.");
		return false;
	}
	if(m_nCells <= 0) {
		throw ModelException("NutRemv","CheckInputData","The dimension of the input data can not be less than zero.");
		return false;
	}
	if(this->m_cellWidth == NULL) {
		throw ModelException("NutRemv","CheckInputData","The relative humidity can not be NULL.");
		return false;
	}
	///...
	return true;
}
void NutrientRemviaSr::SetValue(const char* key, float value)
{
	string sk(key);
	if (StringMatch(sk, VAR_OMP_THREADNUM)) {
		omp_set_num_threads((int)value);
	} 
	else if (StringMatch(sk, Tag_CellSize)) {
		this -> m_nCells = value;
	}
	else if (StringMatch(sk, Tag_CellWidth)) {
		this -> m_cellWidth = value;
	}
	else if (StringMatch(sk, VAR_QTILE)) {
		this -> m_qtile = value;
	}
	else if (StringMatch(sk, VAR_PHOSKD)) {
		this -> m_phoskd = value;
	}
	else {
		throw ModelException("NutRemv","SetValue","Parameter " + sk + " does not exist in CLIMATE method. Please contact the module developer.");
	}
}
void NutrientRemviaSr::Set1DData(const char* key,int n, float *data)
{
	if(!this->CheckInputSize(key,n)) return;

	string sk(key);
	if (StringMatch(sk, VAR_SOER)) {
		this -> m_surfr = data;
	}
	else if (StringMatch(sk, VAR_ANION_EXCL)) {
		this -> m_anion_excl = data;
	}
	else if (StringMatch(sk, VAR_NPERCO)) {
		this -> m_nperco = data;
	}
	else if (StringMatch(sk, VAR_PPERCO)) {
		this -> m_pperco = data;
	}
	else if (StringMatch(sk, VAR_LDRAIN)) {
		this -> m_ldrain = data;
	}
	else if (StringMatch(sk, VAR_ISEP_OPT)) {
		this -> m_isep_opt = data;
	}
	else {
		throw ModelException("NutRemv","SetValue","Parameter " + sk + " does not exist in CLIMATE module. Please contact the module developer.");
	}
}
void NutrientRemviaSr::Set2DData(const char* key, int nRows, int nCols, float** data)
{
	if(!this->CheckInputSize(key,nCols)) return;

	string sk(key);
	if (StringMatch(sk, VAR_ROOTDEPTH)) {
		this -> m_flat = data;
	}
	else if (StringMatch(sk, VAR_SOL_NO3)) {
		this -> m_sol_no3 = data;
	}
	else if (StringMatch(sk, VAR_SOL_BD)) {
		this -> m_sol_bd = data;
	}
	else if (StringMatch(sk, VAR_SOL_SOLP)) {
		this -> m_sol_solp = data;
	}
	else if (StringMatch(sk, VAR_CONV_WT)) {
		this -> m_conv_wt = data;
	}
	else if (StringMatch(sk, VAR_SOL_PERCO)) {
		this -> m_sol_perco = data;
	}
	else if (StringMatch(sk, VAR_SOL_WSATUR)) {
		this -> m_sol_wsatur = data;
	}
	else {
		throw ModelException("NutRemv","SetValue","Parameter " + sk + " does not exist in CLIMATE module. Please contact the module developer.");
	}
}
int NutrientRemviaSr::Execute() {
	if(!this -> CheckInputData()) { 
		return false;
	}
	//Calculate the loss of nitrate via surface runoff, lateral flow, tile flow, and percolation out of the profile.
	Nitrateloss();
	// Calculates the amount of phosphorus lost from the soil.
	Phosphorusloss();
	//return ??
	return 0;
}
void NutrientRemviaSr::Nitrateloss(){
	//percnlyr nitrate moved to next lower layer with percolation (kg/km2)
	float percnlyr = 0.;
	float* tileno3;
	float total_no3 = 0.;
	// Calculate total no3
	for(int i = 0; i < m_nCells; i++) {
		for(int k = 0; k < m_nSolLyrs; k++) {
			total_no3 += m_sol_no3[i][k];
		}
	}

	for(int i = 0; i < m_nCells; i++) {
		for(int k = 0; k < m_nSolLyrs; k++) {
			//add nitrate moved from layer above
			m_sol_no3[i][k] = m_sol_no3[i][k] + percnlyr;
			if (m_sol_no3[i][k] < 1e-6) {
				m_sol_no3[i][k] = 0.0;
			}
			// determine concentration of nitrate in mobile water
			// surface runoff generated (sro)
			float sro = 0.;
			// amount of mobile water in the layer (mw)
			float mw = 0.;
			float vno3 = 0.;
			float con = 0.;
			float ww = 0.;

			if (k == 0) {
				sro = m_surfr[i];
			} else {
				sro = 0.;
			}
			if (m_ldrain[i] == k) {
				mw = mw + m_qtile;
			}
			// Calculate the concentration of nitrate in the mobile water (con), 
			// equation 4:2.1.2, 4:2.1.3 and 4:2.1.4 in SWAT Theory 2009, p269
			mw = m_sol_perco[i][k] + sro + m_flat[i][k] + 1.e-10;
			ww = -mw / ((1. - m_anion_excl[i]) * m_sol_wsatur[i][k]);
			vno3 = m_sol_no3[i][k] * (1. - exp(ww));
			if (mw > 1.e-10) {
				con = max(vno3 / mw, 0.);
			}

			// calculate nitrate in surface runoff
			// concentration of nitrate in surface runoff (cosurf)
			float cosurf = 0.;
			if (m_isep_opt[i] == 2) {
				cosurf = 1.0 * con; // N percolation does not apply to failing septic place;
			} else {
				cosurf = m_nperco[i] * con;
			}
			if (k == 0) {
				m_surqno3[i] = m_surfr[i] * cosurf;
				m_surqno3[i] = min(m_surqno3[i], m_sol_no3[i][k]);
				m_sol_no3[i][k] = m_sol_no3[i][k] - m_surqno3[i];
			}   

			// calculate nitrate in tile flow 
			if (m_ldrain[i] == k) {
					// m_alph_e[i] = exp(-1./(m_n_lag[i] + 1.e-6))
					// ww1 = -1./ ((1. - m_anion_excl[i]) * m_sol_wsatur[i][k])
					// m_vno3_c = m_sol_no3[i][k] * (1. - exp(ww1))
					// if (total_no3 > 1.001) {
					//	 tno3ln = n_lnco[i] * (log(total_no3)) ** m_n_ln[i]
					// else
					//	 tno3ln = 0.
					// }
					// vno3_c = tno3ln * (1. - Exp(ww1))
					// co_p[i] = co_p[i] * (1. - alph_e[i]) + vno3_c * alph_e[i]
				tileno3[i] = con * m_qtile;
				tileno3[i] = min(tileno3[i], m_sol_no3[i][k]);
				m_sol_no3[i][k] = m_sol_no3[i][k] - tileno3[i];       
			}
			// calculate nitrate in lateral flow
			// nitrate transported in lateral flow from layer (ssfnlyr)
			float ssfnlyr = 0.;
			if (k == 1) {
				ssfnlyr = cosurf * m_flat[i][k];
			} else {
				ssfnlyr = con * m_flat[i][k];
			}
			ssfnlyr = min(ssfnlyr, m_sol_no3[i][k]);
			m_latno3[i] = m_latno3[i] + ssfnlyr;
			m_sol_no3[i][k] = m_sol_no3[i][k] - ssfnlyr;

			// calculate nitrate in percolate
			float percnlyr = 0.;
			percnlyr = con * m_sol_perco[i][k];
			percnlyr = min(percnlyr, m_sol_no3[i][k]);
			m_sol_no3[i][k] = m_sol_no3[i][k] - percnlyr;
		}

		// calculate nitrate leaching from soil profile
		m_percn[i] = percnlyr;
		float nloss = 0.;
		// average distance to the stream(m), default is 35m.
		float dis_stream = 35.0;
		nloss = (2.18 * dis_stream - 8.63) / 100.;
		nloss = max(0.,nloss);
		nloss = min(1.,nloss);
		m_latno3[i] = (1. - nloss) * m_latno3[i];
	}
}
void NutrientRemviaSr::Phosphorusloss(){

	for(int i = 0; i < m_nCells; i++) {
		for(int k = 0; k < m_nSolLyrs; k++) {

		}
	}
}
void NutrientRemviaSr::Get1DData(const char* key, int* n, float** data) {
	string sk(key);
	*n = m_nCells;
	if (StringMatch(sk, VAR_LATNO3)) {
		*data = this -> m_latno3;
	}
	if (StringMatch(sk, VAR_PERCN)) {
		*data = this -> m_percn;
	}
	if (StringMatch(sk, VAR_SURQNO3)) {
		*data = this -> m_surqno3;
	}
	if (StringMatch(sk, VAR_SURQSOLP)) {
		*data = this -> m_surqsolp;
	}
	if (StringMatch(sk, VAR_WSHD_PLCH)) {
		*data = this -> m_wshd_plch;
	}
	else {
		throw ModelException("NutRemv", "GetValue","Parameter " + sk + " does not exist. Please contact the module developer.");
	}
}
void NutrientRemviaSr::Get2DData(const char* key, int *nRows, int *nCols, float*** data) {
	string sk(key);
	*nRows = m_nSolLyrs;
	*nCols = m_nCells;
	if (StringMatch(sk, VAR_SOL_NO3)) {
		*data = this -> m_sol_no3; 
	}
	if (StringMatch(sk, VAR_SOL_SOLP)) {
		*data = this -> m_sol_solp;
	}
	else
		throw ModelException("Denitrification", "Get2DData", "Output " + sk 
		+ " does not exist in the Denitrification module. Please contact the module developer.");
}
// int main() {
	//	system("pause");
//}