/*!
 * \file api.cpp
 * \brief Define MetadataInfo of NutRemv module.
/*!
 * \file api.cpp
 * \ingroup NutRemv
 * \author Huiran Gao
 * \date May 2016
 */


#include <stdio.h>
#include <string>
#include "api.h"
#include "util.h"
#include "NutrientRemviaSr.h"
#include <iostream>
#include "SimulationModule.h"
#include "MetadataInfo.h"
#include "MetadataInfoConst.h"

extern "C" SEIMS_MODULE_API SimulationModule* GetInstance()
{
	return new NutrientRemviaSr();
}

//! function to return the XML Metadata document string
extern "C" SEIMS_MODULE_API const char* MetadataInformation()
{
	MetadataInfo mdi;
	mdi.SetAuthor("Huiran Gao");
	mdi.SetClass(MCLS_NutRemv, MCLSDESC_NutRemv);
	mdi.SetDescription(MDESC_NutRemv);
	mdi.SetEmail(SEIMS_EMAIL);
	mdi.SetID(MDESC_NutRemv);
	mdi.SetName(MDESC_NutRemv);
	mdi.SetVersion("1.0");
	mdi.SetWebsite(SEIMS_SITE);
	mdi.SetHelpfile("NutRemv.html");

	// set the parameters
	mdi.AddParameter(Tag_CellSize, UNIT_NON_DIM, DESC_CellSize, Source_ParameterDB, DT_Single);
	mdi.AddParameter(Tag_CellWidth, UNIT_LEN_M, DESC_CellWidth, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_QTILE, UNIT_NON_DIM, DESC_QTILE, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_WSHD_PLCH, UNIT_CONT_KGKM2, DESC_WSHD_PLCH, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_PHOSKD, UNIT_NON_DIM, DESC_PHOSKD, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_PPERCO, UNIT_NON_DIM, DESC_PPERCO, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_NPERCO, UNIT_NON_DIM, DESC_NPERCO, Source_ParameterDB, DT_Single);
	mdi.AddParameter(VAR_ISEP_OPT, UNIT_NON_DIM, DESC_ISEP_OPT, Source_ParameterDB, DT_Single);

	mdi.AddParameter(VAR_ANION_EXCL, UNIT_NON_DIM, DESC_ANION_EXCL, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_LDRAIN, UNIT_NON_DIM, DESC_LDRAIN, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_SOL_CRK, UNIT_NON_DIM, DESC_SOL_CRK, Source_ParameterDB, DT_Raster1D);
	mdi.AddParameter(VAR_SOL_SUMSAT, UNIT_DEPTH_MM, DESC_SOL_SUMSAT, Source_ParameterDB, DT_Raster1D);

	mdi.AddParameter(VAR_FLAT, UNIT_DEPTH_MM, DESC_FLAT, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_SOL_PERCO, UNIT_DEPTH_MM, DESC_SOL_PERCO, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_ROOTDEPTH, UNIT_DEPTH_MM, DESC_ROOTDEPTH, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_SOL_BD, UNIT_DENSITY, DESC_SOL_BD, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_SOILDEPTH, UNIT_NON_DIM, DESC_SOILDEPTH, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_SOL_NO3, UNIT_CONT_KGKM2, DESC_SOL_NO3, Source_ParameterDB, DT_Raster2D);
	mdi.AddParameter(VAR_SOL_SOLP, UNIT_CONT_KGKM2, DESC_SOL_SOLP, Source_ParameterDB, DT_Raster2D);

	// set the input
	mdi.AddInput(VAR_SURU, UNIT_DEPTH_MM, DESC_SURU, Source_Module, DT_Raster1D);

	// set the output variables
	mdi.AddOutput(VAR_LATNO3, UNIT_CONT_KGKM2, DESC_LATNO3, DT_Raster1D);
	mdi.AddOutput(VAR_PERCN, UNIT_CONT_KGKM2, DESC_PERCN, DT_Raster1D);
	mdi.AddOutput(VAR_SURQNO3, UNIT_CONT_KGKM2, DESC_SURQNO3, DT_Raster1D);
	mdi.AddOutput(VAR_SURQSOLP, UNIT_CONT_KGKM2, DESC_SURQSOLP, DT_Raster1D);

	mdi.AddOutput(VAR_WSHD_PLCH, UNIT_CONT_KGKM2, DESC_WSHD_PLCH, DT_Single);

	mdi.AddOutput(VAR_SOL_NO3, UNIT_CONT_KGKM2, DESC_SOL_NO3, DT_Raster2D);
	mdi.AddOutput(VAR_SOL_SOLP, UNIT_CONT_KGKM2, DESC_SOL_SOLP, DT_Raster2D);

	string res = mdi.GetXMLDocument();
	char* tmp = new char[res.size()+1];
	strprintf(tmp, res.size()+1, "%s", res.c_str());
	return tmp;
}
