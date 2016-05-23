/*!
 * \file api.cpp
 * \brief 
 *
 *
 *
 * \author JunZhi Liu
 * \version 
 * \date June 2015
 *
 * 
 */
#include <stdio.h>
#include <string>
#include "api.h"
#include "util.h"
#include "DepressionFSDaily.h"
#include <iostream>
#include "SimulationModule.h"
#include "MetadataInfo.h"
#include "MetadataInfoConst.h"

/** \defgroup DEP_LINSLEY
 * \ingroup Hydrology_longterm
 * \brief 
 *
 *
 *
 */
//! Get instance of SimulationModule class
extern "C" SEIMS_MODULE_API SimulationModule* GetInstance()
{
	return new DepressionFSDaily();
}

/*!
 * \ingroup DEP_LINSLEY
 * \brief function to return the XML Metadata document string
 *
 *   
 * 
 */
extern "C" SEIMS_MODULE_API const char* MetadataInformation()
{
	MetadataInfo mdi;

	// set the information properties
	mdi.SetAuthor("Junzhi Liu");
	mdi.SetClass(MCLS_DEP, MCLSDESC_DEP);
	mdi.SetDescription(MDESC_DEP_LINSLEY);
	mdi.SetEmail(SEIMS_EMAIL);
	mdi.SetHelpfile("DEP_LINSLEY.chm");
	mdi.SetID(MID_DEP_LINSLEY);
	mdi.SetName(MID_DEP_LINSLEY);
	mdi.SetVersion("0.1");
	mdi.SetWebsite(SEIMS_SITE);

	mdi.AddParameter(Tag_CellSize, UNIT_NON_DIM, DESC_CellSize, Source_ParameterDB, DT_Single);
	//mdi.AddParameter(Tag_CellWidth, UNIT_LEN_M, DESC_CellWidth, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_DEPREIN, UNIT_NON_DIM, DESC_DEPREIN, Source_ParameterDB, DT_Single); 
	mdi.AddParameter(VAR_DEPRESSION,UNIT_DEPTH_MM,DESC_DEPRESSION,Source_ParameterDB, DT_Raster1D);
	mdi.AddInput(VAR_INET, UNIT_DEPTH_MM, DESC_INET, Source_Module, DT_Raster1D);	//EI
	mdi.AddInput(VAR_PET, UNIT_DEPTH_MM, DESC_PET, Source_Module, DT_Raster1D);							//PET
	//mdi.AddInput(VAR_D_INFIL, UNIT_DEPTH_MM, DESC_D_INFIL, Source_Module, DT_Raster);							//Infiltration
	mdi.AddInput(VAR_EXCP, UNIT_DEPTH_MM, DESC_EXCP, Source_Module, DT_Raster1D);						//PE

	mdi.AddOutput(VAR_DPST, UNIT_DEPTH_MM, DESC_DPST, DT_Raster1D);
	mdi.AddOutput(VAR_DEET, UNIT_DEPTH_MM, DESC_DEET, DT_Raster1D);
	mdi.AddOutput(VAR_SURU, UNIT_DEPTH_MM, DESC_SURU, DT_Raster1D);

	// set the dependencies
	mdi.AddDependency("Interpolation","Interpolation module");
	mdi.AddDependency("Interception","Interception module");
	mdi.AddDependency("Infiltration","Infiltration module");

	string res = mdi.GetXMLDocument();

	char* tmp = new char[res.size()+1];
	strprintf(tmp, res.size()+1, "%s", res.c_str());
	return tmp;
}