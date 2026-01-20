
#include <algorithm>
#include <iostream>
#include <vector>

#include "presets.h"

/// @note  This file has been auto-generated. Local changes will not be saved!

#include "restankNoUA.h"
#include "restankHugeUA.h"
#include "restankRealistic.h"
#include "basicIntegrated.h"
#include "AOSmithCAHP120.h"
#include "AOSmithHPTS40.h"
#include "AOSmithHPTS50.h"
#include "AOSmithHPTS66.h"
#include "AOSmithHPTS80.h"
#include "AOSmithHPTU50.h"
#include "AOSmithHPTU66.h"
#include "AOSmithHPTU80.h"
#include "AOSmithHPTU80_DR.h"
#include "AOSmithPHPT60.h"
#include "AOSmithPHPT80.h"
#include "AquaThermAire.h"
#include "AWHSTier3Generic40.h"
#include "AWHSTier3Generic50.h"
#include "AWHSTier3Generic65.h"
#include "AWHSTier3Generic80.h"
#include "AWHSTier4Generic40.h"
#include "AWHSTier4Generic50.h"
#include "AWHSTier4Generic65.h"
#include "AWHSTier4Generic80.h"
#include "BWC2020_65.h"
#include "GE2012_50.h"
#include "GE2014STDMode_50.h"
#include "GE2014STDMode_80.h"
#include "GE2014_50.h"
#include "GE2014_80.h"
#include "GE2014_80DR.h"
#include "Generic1.h"
#include "Generic2.h"
#include "Generic3.h"
#include "GenericUEF217.h"
#include "Rheem2020Build40.h"
#include "Rheem2020Build50.h"
#include "Rheem2020Build65.h"
#include "Rheem2020Build80.h"
#include "Rheem2020Prem40.h"
#include "Rheem2020Prem50.h"
#include "Rheem2020Prem65.h"
#include "Rheem2020Prem80.h"
#include "RheemHB50.h"
#include "RheemHBDR2250.h"
#include "RheemHBDR2265.h"
#include "RheemHBDR2280.h"
#include "RheemHBDR4550.h"
#include "RheemHBDR4565.h"
#include "RheemHBDR4580.h"
#include "RheemPlugInDedicated40.h"
#include "RheemPlugInDedicated50.h"
#include "RheemPlugInShared40.h"
#include "RheemPlugInShared50.h"
#include "RheemPlugInShared65.h"
#include "RheemPlugInShared80.h"
#include "Stiebel220E.h"
#include "UEF2generic.h"
#include "LG_APHWC50.h"
#include "LG_APHWC80.h"
#include "BradfordWhiteAeroThermRE2H50.h"
#include "BradfordWhiteAeroThermRE2H65.h"
#include "BradfordWhiteAeroThermRE2H80.h"
#include "StorageTank.h"
#include "Sanco43.h"
#include "Sanco83.h"
#include "SancoGS3_45HPA_US_SP.h"
#include "Sanco119.h"
#include "TamScalable_SP.h"
#include "TamScalable_SP_2X.h"
#include "TamScalable_SP_Half.h"
#include "Scalable_MP.h"
#include "Mitsubishi_QAHV_N136TAU_HPB_SP.h"
#include "ColmacCxV_5_SP.h"
#include "ColmacCxA_10_SP.h"
#include "ColmacCxA_15_SP.h"
#include "ColmacCxA_20_SP.h"
#include "ColmacCxA_25_SP.h"
#include "ColmacCxA_30_SP.h"
#include "ColmacCxV_5_MP.h"
#include "ColmacCxA_10_MP.h"
#include "ColmacCxA_15_MP.h"
#include "ColmacCxA_20_MP.h"
#include "ColmacCxA_25_MP.h"
#include "ColmacCxA_30_MP.h"
#include "NyleC25A_SP.h"
#include "NyleC60A_SP.h"
#include "NyleC90A_SP.h"
#include "NyleC125A_SP.h"
#include "NyleC185A_SP.h"
#include "NyleC250A_SP.h"
#include "NyleC60A_C_SP.h"
#include "NyleC90A_C_SP.h"
#include "NyleC125A_C_SP.h"
#include "NyleC185A_C_SP.h"
#include "NyleC250A_C_SP.h"
#include "NyleC60A_MP.h"
#include "NyleC90A_MP.h"
#include "NyleC125A_MP.h"
#include "NyleC185A_MP.h"
#include "NyleC250A_MP.h"
#include "NyleC60A_C_MP.h"
#include "NyleC90A_C_MP.h"
#include "NyleC125A_C_MP.h"
#include "NyleC185A_C_MP.h"
#include "NyleC250A_C_MP.h"
#include "RheemHPHD60.h"
#include "RheemHPHD135.h"

namespace hpwh_presets {

// clang-format off
std::vector<Model> models({
	{ MODELS::restankNoUA, "restankNoUA", cbor_restankNoUA.data(), sizeof(cbor_restankNoUA)},
	{ MODELS::restankHugeUA, "restankHugeUA", cbor_restankHugeUA.data(), sizeof(cbor_restankHugeUA)},
	{ MODELS::restankRealistic, "restankRealistic", cbor_restankRealistic.data(), sizeof(cbor_restankRealistic)},
	{ MODELS::basicIntegrated, "basicIntegrated", cbor_basicIntegrated.data(), sizeof(cbor_basicIntegrated)},
	{ MODELS::AOSmithCAHP120, "AOSmithCAHP120", cbor_AOSmithCAHP120.data(), sizeof(cbor_AOSmithCAHP120)},
	{ MODELS::AOSmithHPTS40, "AOSmithHPTS40", cbor_AOSmithHPTS40.data(), sizeof(cbor_AOSmithHPTS40)},
	{ MODELS::AOSmithHPTS50, "AOSmithHPTS50", cbor_AOSmithHPTS50.data(), sizeof(cbor_AOSmithHPTS50)},
	{ MODELS::AOSmithHPTS66, "AOSmithHPTS66", cbor_AOSmithHPTS66.data(), sizeof(cbor_AOSmithHPTS66)},
	{ MODELS::AOSmithHPTS80, "AOSmithHPTS80", cbor_AOSmithHPTS80.data(), sizeof(cbor_AOSmithHPTS80)},
	{ MODELS::AOSmithHPTU50, "AOSmithHPTU50", cbor_AOSmithHPTU50.data(), sizeof(cbor_AOSmithHPTU50)},
	{ MODELS::AOSmithHPTU66, "AOSmithHPTU66", cbor_AOSmithHPTU66.data(), sizeof(cbor_AOSmithHPTU66)},
	{ MODELS::AOSmithHPTU80, "AOSmithHPTU80", cbor_AOSmithHPTU80.data(), sizeof(cbor_AOSmithHPTU80)},
	{ MODELS::AOSmithHPTU80_DR, "AOSmithHPTU80_DR", cbor_AOSmithHPTU80_DR.data(), sizeof(cbor_AOSmithHPTU80_DR)},
	{ MODELS::AOSmithPHPT60, "AOSmithPHPT60", cbor_AOSmithPHPT60.data(), sizeof(cbor_AOSmithPHPT60)},
	{ MODELS::AOSmithPHPT80, "AOSmithPHPT80", cbor_AOSmithPHPT80.data(), sizeof(cbor_AOSmithPHPT80)},
	{ MODELS::AquaThermAire, "AquaThermAire", cbor_AquaThermAire.data(), sizeof(cbor_AquaThermAire)},
	{ MODELS::AWHSTier3Generic40, "AWHSTier3Generic40", cbor_AWHSTier3Generic40.data(), sizeof(cbor_AWHSTier3Generic40)},
	{ MODELS::AWHSTier3Generic50, "AWHSTier3Generic50", cbor_AWHSTier3Generic50.data(), sizeof(cbor_AWHSTier3Generic50)},
	{ MODELS::AWHSTier3Generic65, "AWHSTier3Generic65", cbor_AWHSTier3Generic65.data(), sizeof(cbor_AWHSTier3Generic65)},
	{ MODELS::AWHSTier3Generic80, "AWHSTier3Generic80", cbor_AWHSTier3Generic80.data(), sizeof(cbor_AWHSTier3Generic80)},
	{ MODELS::AWHSTier4Generic40, "AWHSTier4Generic40", cbor_AWHSTier4Generic40.data(), sizeof(cbor_AWHSTier4Generic40)},
	{ MODELS::AWHSTier4Generic50, "AWHSTier4Generic50", cbor_AWHSTier4Generic50.data(), sizeof(cbor_AWHSTier4Generic50)},
	{ MODELS::AWHSTier4Generic65, "AWHSTier4Generic65", cbor_AWHSTier4Generic65.data(), sizeof(cbor_AWHSTier4Generic65)},
	{ MODELS::AWHSTier4Generic80, "AWHSTier4Generic80", cbor_AWHSTier4Generic80.data(), sizeof(cbor_AWHSTier4Generic80)},
	{ MODELS::BWC2020_65, "BWC2020_65", cbor_BWC2020_65.data(), sizeof(cbor_BWC2020_65)},
	{ MODELS::GE2012_50, "GE2012_50", cbor_GE2012_50.data(), sizeof(cbor_GE2012_50)},
	{ MODELS::GE2014STDMode_50, "GE2014STDMode_50", cbor_GE2014STDMode_50.data(), sizeof(cbor_GE2014STDMode_50)},
	{ MODELS::GE2014STDMode_80, "GE2014STDMode_80", cbor_GE2014STDMode_80.data(), sizeof(cbor_GE2014STDMode_80)},
	{ MODELS::GE2014_50, "GE2014_50", cbor_GE2014_50.data(), sizeof(cbor_GE2014_50)},
	{ MODELS::GE2014_80, "GE2014_80", cbor_GE2014_80.data(), sizeof(cbor_GE2014_80)},
	{ MODELS::GE2014_80DR, "GE2014_80DR", cbor_GE2014_80DR.data(), sizeof(cbor_GE2014_80DR)},
	{ MODELS::Generic1, "Generic1", cbor_Generic1.data(), sizeof(cbor_Generic1)},
	{ MODELS::Generic2, "Generic2", cbor_Generic2.data(), sizeof(cbor_Generic2)},
	{ MODELS::Generic3, "Generic3", cbor_Generic3.data(), sizeof(cbor_Generic3)},
	{ MODELS::GenericUEF217, "GenericUEF217", cbor_GenericUEF217.data(), sizeof(cbor_GenericUEF217)},
	{ MODELS::Rheem2020Build40, "Rheem2020Build40", cbor_Rheem2020Build40.data(), sizeof(cbor_Rheem2020Build40)},
	{ MODELS::Rheem2020Build50, "Rheem2020Build50", cbor_Rheem2020Build50.data(), sizeof(cbor_Rheem2020Build50)},
	{ MODELS::Rheem2020Build65, "Rheem2020Build65", cbor_Rheem2020Build65.data(), sizeof(cbor_Rheem2020Build65)},
	{ MODELS::Rheem2020Build80, "Rheem2020Build80", cbor_Rheem2020Build80.data(), sizeof(cbor_Rheem2020Build80)},
	{ MODELS::Rheem2020Prem40, "Rheem2020Prem40", cbor_Rheem2020Prem40.data(), sizeof(cbor_Rheem2020Prem40)},
	{ MODELS::Rheem2020Prem50, "Rheem2020Prem50", cbor_Rheem2020Prem50.data(), sizeof(cbor_Rheem2020Prem50)},
	{ MODELS::Rheem2020Prem65, "Rheem2020Prem65", cbor_Rheem2020Prem65.data(), sizeof(cbor_Rheem2020Prem65)},
	{ MODELS::Rheem2020Prem80, "Rheem2020Prem80", cbor_Rheem2020Prem80.data(), sizeof(cbor_Rheem2020Prem80)},
	{ MODELS::RheemHB50, "RheemHB50", cbor_RheemHB50.data(), sizeof(cbor_RheemHB50)},
	{ MODELS::RheemHBDR2250, "RheemHBDR2250", cbor_RheemHBDR2250.data(), sizeof(cbor_RheemHBDR2250)},
	{ MODELS::RheemHBDR2265, "RheemHBDR2265", cbor_RheemHBDR2265.data(), sizeof(cbor_RheemHBDR2265)},
	{ MODELS::RheemHBDR2280, "RheemHBDR2280", cbor_RheemHBDR2280.data(), sizeof(cbor_RheemHBDR2280)},
	{ MODELS::RheemHBDR4550, "RheemHBDR4550", cbor_RheemHBDR4550.data(), sizeof(cbor_RheemHBDR4550)},
	{ MODELS::RheemHBDR4565, "RheemHBDR4565", cbor_RheemHBDR4565.data(), sizeof(cbor_RheemHBDR4565)},
	{ MODELS::RheemHBDR4580, "RheemHBDR4580", cbor_RheemHBDR4580.data(), sizeof(cbor_RheemHBDR4580)},
	{ MODELS::RheemPlugInDedicated40, "RheemPlugInDedicated40", cbor_RheemPlugInDedicated40.data(), sizeof(cbor_RheemPlugInDedicated40)},
	{ MODELS::RheemPlugInDedicated50, "RheemPlugInDedicated50", cbor_RheemPlugInDedicated50.data(), sizeof(cbor_RheemPlugInDedicated50)},
	{ MODELS::RheemPlugInShared40, "RheemPlugInShared40", cbor_RheemPlugInShared40.data(), sizeof(cbor_RheemPlugInShared40)},
	{ MODELS::RheemPlugInShared50, "RheemPlugInShared50", cbor_RheemPlugInShared50.data(), sizeof(cbor_RheemPlugInShared50)},
	{ MODELS::RheemPlugInShared65, "RheemPlugInShared65", cbor_RheemPlugInShared65.data(), sizeof(cbor_RheemPlugInShared65)},
	{ MODELS::RheemPlugInShared80, "RheemPlugInShared80", cbor_RheemPlugInShared80.data(), sizeof(cbor_RheemPlugInShared80)},
	{ MODELS::Stiebel220E, "Stiebel220E", cbor_Stiebel220E.data(), sizeof(cbor_Stiebel220E)},
	{ MODELS::UEF2generic, "UEF2generic", cbor_UEF2generic.data(), sizeof(cbor_UEF2generic)},
	{ MODELS::LG_APHWC50, "LG_APHWC50", cbor_LG_APHWC50.data(), sizeof(cbor_LG_APHWC50)},
	{ MODELS::LG_APHWC80, "LG_APHWC80", cbor_LG_APHWC80.data(), sizeof(cbor_LG_APHWC80)},
	{ MODELS::BradfordWhiteAeroThermRE2H50, "BradfordWhiteAeroThermRE2H50", cbor_BradfordWhiteAeroThermRE2H50.data(), sizeof(cbor_BradfordWhiteAeroThermRE2H50)},
	{ MODELS::BradfordWhiteAeroThermRE2H65, "BradfordWhiteAeroThermRE2H65", cbor_BradfordWhiteAeroThermRE2H65.data(), sizeof(cbor_BradfordWhiteAeroThermRE2H65)},
	{ MODELS::BradfordWhiteAeroThermRE2H80, "BradfordWhiteAeroThermRE2H80", cbor_BradfordWhiteAeroThermRE2H80.data(), sizeof(cbor_BradfordWhiteAeroThermRE2H80)},
	{ MODELS::StorageTank, "StorageTank", cbor_StorageTank.data(), sizeof(cbor_StorageTank)},
	{ MODELS::Sanco43, "Sanco43", cbor_Sanco43.data(), sizeof(cbor_Sanco43)},
	{ MODELS::Sanco83, "Sanco83", cbor_Sanco83.data(), sizeof(cbor_Sanco83)},
	{ MODELS::SancoGS3_45HPA_US_SP, "SancoGS3_45HPA_US_SP", cbor_SancoGS3_45HPA_US_SP.data(), sizeof(cbor_SancoGS3_45HPA_US_SP)},
	{ MODELS::Sanco119, "Sanco119", cbor_Sanco119.data(), sizeof(cbor_Sanco119)},
	{ MODELS::TamScalable_SP, "TamScalable_SP", cbor_TamScalable_SP.data(), sizeof(cbor_TamScalable_SP)},
	{ MODELS::TamScalable_SP_2X, "TamScalable_SP_2X", cbor_TamScalable_SP_2X.data(), sizeof(cbor_TamScalable_SP_2X)},
	{ MODELS::TamScalable_SP_Half, "TamScalable_SP_Half", cbor_TamScalable_SP_Half.data(), sizeof(cbor_TamScalable_SP_Half)},
	{ MODELS::Scalable_MP, "Scalable_MP", cbor_Scalable_MP.data(), sizeof(cbor_Scalable_MP)},
	{ MODELS::Mitsubishi_QAHV_N136TAU_HPB_SP, "Mitsubishi_QAHV_N136TAU_HPB_SP", cbor_Mitsubishi_QAHV_N136TAU_HPB_SP.data(), sizeof(cbor_Mitsubishi_QAHV_N136TAU_HPB_SP)},
	{ MODELS::ColmacCxV_5_SP, "ColmacCxV_5_SP", cbor_ColmacCxV_5_SP.data(), sizeof(cbor_ColmacCxV_5_SP)},
	{ MODELS::ColmacCxA_10_SP, "ColmacCxA_10_SP", cbor_ColmacCxA_10_SP.data(), sizeof(cbor_ColmacCxA_10_SP)},
	{ MODELS::ColmacCxA_15_SP, "ColmacCxA_15_SP", cbor_ColmacCxA_15_SP.data(), sizeof(cbor_ColmacCxA_15_SP)},
	{ MODELS::ColmacCxA_20_SP, "ColmacCxA_20_SP", cbor_ColmacCxA_20_SP.data(), sizeof(cbor_ColmacCxA_20_SP)},
	{ MODELS::ColmacCxA_25_SP, "ColmacCxA_25_SP", cbor_ColmacCxA_25_SP.data(), sizeof(cbor_ColmacCxA_25_SP)},
	{ MODELS::ColmacCxA_30_SP, "ColmacCxA_30_SP", cbor_ColmacCxA_30_SP.data(), sizeof(cbor_ColmacCxA_30_SP)},
	{ MODELS::ColmacCxV_5_MP, "ColmacCxV_5_MP", cbor_ColmacCxV_5_MP.data(), sizeof(cbor_ColmacCxV_5_MP)},
	{ MODELS::ColmacCxA_10_MP, "ColmacCxA_10_MP", cbor_ColmacCxA_10_MP.data(), sizeof(cbor_ColmacCxA_10_MP)},
	{ MODELS::ColmacCxA_15_MP, "ColmacCxA_15_MP", cbor_ColmacCxA_15_MP.data(), sizeof(cbor_ColmacCxA_15_MP)},
	{ MODELS::ColmacCxA_20_MP, "ColmacCxA_20_MP", cbor_ColmacCxA_20_MP.data(), sizeof(cbor_ColmacCxA_20_MP)},
	{ MODELS::ColmacCxA_25_MP, "ColmacCxA_25_MP", cbor_ColmacCxA_25_MP.data(), sizeof(cbor_ColmacCxA_25_MP)},
	{ MODELS::ColmacCxA_30_MP, "ColmacCxA_30_MP", cbor_ColmacCxA_30_MP.data(), sizeof(cbor_ColmacCxA_30_MP)},
	{ MODELS::NyleC25A_SP, "NyleC25A_SP", cbor_NyleC25A_SP.data(), sizeof(cbor_NyleC25A_SP)},
	{ MODELS::NyleC60A_SP, "NyleC60A_SP", cbor_NyleC60A_SP.data(), sizeof(cbor_NyleC60A_SP)},
	{ MODELS::NyleC90A_SP, "NyleC90A_SP", cbor_NyleC90A_SP.data(), sizeof(cbor_NyleC90A_SP)},
	{ MODELS::NyleC125A_SP, "NyleC125A_SP", cbor_NyleC125A_SP.data(), sizeof(cbor_NyleC125A_SP)},
	{ MODELS::NyleC185A_SP, "NyleC185A_SP", cbor_NyleC185A_SP.data(), sizeof(cbor_NyleC185A_SP)},
	{ MODELS::NyleC250A_SP, "NyleC250A_SP", cbor_NyleC250A_SP.data(), sizeof(cbor_NyleC250A_SP)},
	{ MODELS::NyleC60A_C_SP, "NyleC60A_C_SP", cbor_NyleC60A_C_SP.data(), sizeof(cbor_NyleC60A_C_SP)},
	{ MODELS::NyleC90A_C_SP, "NyleC90A_C_SP", cbor_NyleC90A_C_SP.data(), sizeof(cbor_NyleC90A_C_SP)},
	{ MODELS::NyleC125A_C_SP, "NyleC125A_C_SP", cbor_NyleC125A_C_SP.data(), sizeof(cbor_NyleC125A_C_SP)},
	{ MODELS::NyleC185A_C_SP, "NyleC185A_C_SP", cbor_NyleC185A_C_SP.data(), sizeof(cbor_NyleC185A_C_SP)},
	{ MODELS::NyleC250A_C_SP, "NyleC250A_C_SP", cbor_NyleC250A_C_SP.data(), sizeof(cbor_NyleC250A_C_SP)},
	{ MODELS::NyleC60A_MP, "NyleC60A_MP", cbor_NyleC60A_MP.data(), sizeof(cbor_NyleC60A_MP)},
	{ MODELS::NyleC90A_MP, "NyleC90A_MP", cbor_NyleC90A_MP.data(), sizeof(cbor_NyleC90A_MP)},
	{ MODELS::NyleC125A_MP, "NyleC125A_MP", cbor_NyleC125A_MP.data(), sizeof(cbor_NyleC125A_MP)},
	{ MODELS::NyleC185A_MP, "NyleC185A_MP", cbor_NyleC185A_MP.data(), sizeof(cbor_NyleC185A_MP)},
	{ MODELS::NyleC250A_MP, "NyleC250A_MP", cbor_NyleC250A_MP.data(), sizeof(cbor_NyleC250A_MP)},
	{ MODELS::NyleC60A_C_MP, "NyleC60A_C_MP", cbor_NyleC60A_C_MP.data(), sizeof(cbor_NyleC60A_C_MP)},
	{ MODELS::NyleC90A_C_MP, "NyleC90A_C_MP", cbor_NyleC90A_C_MP.data(), sizeof(cbor_NyleC90A_C_MP)},
	{ MODELS::NyleC125A_C_MP, "NyleC125A_C_MP", cbor_NyleC125A_C_MP.data(), sizeof(cbor_NyleC125A_C_MP)},
	{ MODELS::NyleC185A_C_MP, "NyleC185A_C_MP", cbor_NyleC185A_C_MP.data(), sizeof(cbor_NyleC185A_C_MP)},
	{ MODELS::NyleC250A_C_MP, "NyleC250A_C_MP", cbor_NyleC250A_C_MP.data(), sizeof(cbor_NyleC250A_C_MP)},
	{ MODELS::RheemHPHD60, "RheemHPHD60", cbor_RheemHPHD60.data(), sizeof(cbor_RheemHPHD60)},
	{ MODELS::RheemHPHD135, "RheemHPHD135", cbor_RheemHPHD135.data(), sizeof(cbor_RheemHPHD135)},
});
// clang-format on

Model find_by_name(const std::string& name)
{
    auto it = std::find_if(models.begin(),
                      models.end(),
                      [&name](Model& model){ return model.name == name; });
    if (it != models.end())
        return *it;
    return {MODELS::unknown, "unknown", nullptr, 0};
}

Model find_by_id(const MODELS id)
{
    auto it =
        std::find_if(models.begin(),
                models.end(),
                [&id](Model& model) { return model.id == id; });
    if (it != models.end())
        return *it;
    return {MODELS::unknown, "unknown", nullptr, 0};
}

}
