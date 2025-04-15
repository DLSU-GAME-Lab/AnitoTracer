#include "FileUtils.h"

#include <algorithm>
#include <commdlg.h>



// #if __cplusplus <= 201402L
// #define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
// #include <experimental/filesystem>
// #endif

//#include "LogUtils.h"

std::filesystem::path FileUtils::getAssetsFolderPath()
{
	std::filesystem::path p = getExecutablePath().parent_path().parent_path().append("assets");
	//std::cout << "Assets folder: " << p << '\n';
	return p;
}

std::filesystem::path FileUtils::getSceneSavePath()
{

	std::filesystem::path p = getExecutablePath().parent_path().parent_path().append("assets/scenes");
	//std::cout << "Assets folder: " << p << '\n';
	return p;

}

std::filesystem::path FileUtils::getExecutablePath()
{
	//return std::filesystem::current_path();
	{
#ifdef _WIN32
		wchar_t path[MAX_PATH] = { 0 };
		GetModuleFileName(nullptr, path, MAX_PATH);
		return path;
#else
		char result[PATH_MAX];
		ssize_t count = readlink("/proc/self/exe" reult, PATH_MAX);
		return std::string(result, (count > 0) ? count : 0);
#endif
	}
}

// Prompts the user to open a 3D model file. Supply a reference to a filePath and fileName string for the result.
bool FileUtils::getModelFilePath(std::string& filePath, std::string& fileName)
{
	//gdeng03::LogUtils::log("OpenFile");
	wchar_t path[MAX_PATH] = L"";

	constexpr LPCWSTR fileFormats =
		L"3D Manufacturing Format (.3mf)\0*.3mf\0"
		"Collada (.dae, .xml)\0*.dae;*.xml\0"
		"Blender (.blend)\0*.blend\0"
		"Biovision BVH (.bvh)\0*.bvh\0"
		"3D Studio Max 3DS (.3ds)\0*.3ds\0"
		"3D Studio Max ASE (.ase)\0*.ase\0"
		"glTF, glTF2.0 (.glTF, .glb)\0*.glTF;*.glb\0"
		"FBX - Format, as ASCII and binary (.fbx)\0*.fbx\0"
		"Stanford Polygon Library (.ply)\0*.ply\0"
		"AutoCAD DXF (.dxf)\0*.dxf\0"
		"IFC - STEP (.ifc)\0*.ifc\0"
		"Neutral File Format, Sense8 WorldToolkit (.nff)\0*.nff\0"
		"Valve Model (.smd, .vta)\0*.smd;*.vta\0"
		"Quake I (.mdl)\0*.mdl\0"
		"Quake II (.md2)\0*.md2\0"
		"Quake III (.md3)\0*.md3\0"
		"Quake 3 BSP (.pk3)\0*.pk3\0"
		"RtCW (.mdc)\0*.mdc\0"
		"Doom 3 (.md5mesh, .md5anim, .md5camera)\0*.md5mesh;*.md5anim;*.md5camera\0"
		"DirectX (.x) X\0*.x\0"
		"Quick3D (.q3o, .q3s)\0*.q3o;*.q3s\0"
		"Raw Triangles (.raw)\0*.raw\0"
		"AC3D (.ac, .ac3d)\0*.ac;*.ac3d\0"
		"Stereolithography (.stl)\0*.stl\0"
		"Autodesk DXF (.dxf)\0*.dxf\0"
		"Irrlicht Mesh (.irrmesh, .xml)\0*.irrmesh;*.xml\0"
		"Irrlicht Scene (.irr)\0*.irr;*.xml\0"
		"Object File Format (.off)\0*.off\0"
		"Wavefront Object (.obj)\0*.obj\0"
		"Terragen Terrain (.ter)\0*.ter\0"
		"3D GameStudio Model (.mdl)\0*.mdl\0"
		"3D GameStudio Terrain (.hmp)\0*.hmp\0"
		"Ogre (.mesh.xml, .skeleton.xml, .material)\0*.mesh.xml;*.skeleton.xml;*.material\0"
		"OpenGEX - Fomat (.ogex)\0*.ogex\0"
		"Milkshape 3D (.ms3d)\0*.ms3d\0"
		"LightWave Model (.lwo)\0*.lwo\0"
		"LightWave Scene (.lws)\0*.lws\0"
		"Modo Model (.lxo)\0*.lxo\0"
		"CharacterStudio Motion (.csm)\0*.csm\0"
		"Stanford Ply (.ply)\0*.ply\0"
		"TrueSpace (.cob, .scn)\0*.cob;*.scn\0"
		"XGL - 3D - Format (.xgl)\0*.xgl\0"
		"All Files\0*.*\0";

	OPENFILENAME openFile = OPENFILENAME();
	openFile.lStructSize = sizeof(OPENFILENAME);
	openFile.hwndOwner = nullptr;
	openFile.nMaxFile = MAX_PATH;
	openFile.lpstrFile = path;
	openFile.lpstrFilter = fileFormats;
	openFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&openFile)) {
		std::wstring ws(path);
		std::string str(ws.begin(), ws.end());
		std::ranges::replace(str, '\\', '/');
		fileName = std::filesystem::path(str).stem().generic_string();
		filePath = str;
		return true;
	}

	return false;
}

bool FileUtils::getScenePath(std::string& filePath, std::string& fileName)
{
	wchar_t path[MAX_PATH] = L"";

	constexpr LPCWSTR fileFormats =
		L"JSON (.level)\0*.level\0"
		"TXT (.txt)\0*.txt\0"
		"All Files\0*.*\0";

	OPENFILENAME openFile = OPENFILENAME();
	openFile.lStructSize = sizeof(OPENFILENAME);
	openFile.hwndOwner = nullptr;
	openFile.nMaxFile = MAX_PATH;
	openFile.lpstrFile = path;
	openFile.lpstrFilter = fileFormats;
	openFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&openFile)) {
		std::wstring ws(path);
		std::string str(ws.begin(), ws.end());
		std::ranges::replace(str, '\\', '/');
		fileName = std::filesystem::path(str).stem().generic_string();
		filePath = str;
		return true;
	}

	return false;
}

