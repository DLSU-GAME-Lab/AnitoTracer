#include "Asset/AssetLoading/BatchLoader.h"
#include "Asset/IAsset.h"

#include "Organization/SingletonMacro.hpp"

class AssetPipeline {
	SINGLETON_MACRO_CUSTOM(AssetPipeline);

public:
	static void LoadAssetsFolder();
};