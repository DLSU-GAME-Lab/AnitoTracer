#include "FileUtils.h"

#include <algorithm>
#include <commdlg.h>
#include <fstream>
#include <iostream>

// #if __cplusplus <= 201402L
// #define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
// #include <experimental/filesystem>
// #endif

//#include "LogUtils.h"

void FileUtils::initializeProjectFolder()
{
	if (std::filesystem::create_directory(FileUtils::getProjectFolderPath()))
	{
		std::filesystem::create_directory(FileUtils::getProjectFolderPath().string() + "/Models");
		std::filesystem::create_directory(FileUtils::getProjectFolderPath().string() + "/Textures");
		std::cout << "Assets directory created" << std::endl;
	}
	else
	{
		if (exists(FileUtils::getProjectFolderPath()) && is_directory(FileUtils::getProjectFolderPath())) {
			std::cout << "Assets directory already exists" << std::endl;
		}
		else {
			std::cerr << "Failed to create Assets directory" << std::endl;
		}
	}
}

std::filesystem::path FileUtils::getAssetsFolderPath()
{
	std::filesystem::path p = getExecutablePath().parent_path().parent_path().append("assets");
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


std::filesystem::path FileUtils::getProjectFolderPath() {
	std::filesystem::path p = getExecutablePath().parent_path().append("/Project/");
	return p;
}

// Prompts the user to open a 3D model file. Supply a reference to a filePath and fileName string for the result.
bool FileUtils::getModelFilePath(std::string& filePath, std::string& fileName)
{
	//gdeng03::LogUtils::log("OpenFile");
	wchar_t path[MAX_PATH] = L"";

	constexpr LPCWSTR fileFormats =
		L"All Supported 3D Formats\0*.3mf;*.dae;*.xml;*.blend;*.bvh;*.3ds;*.ase;*.gltf;*.glb;"
		"*.fbx;*.ply;*.dxf;*.ifc;*.nff;*.smd;*.vta;*.mdl;*.md2;"
		"*.md3;*.pk3;*.mdc;*.md5mesh;*.md5anim;*.md5camera;*.x;"
		"*.q3o;*.q3s;*.raw;*.ac;*.ac3d;*.stl;*.irrmesh;*.irr;"
		"*.off;*.obj;*.ter;*.hmp;*.mesh.xml;*.skeleton.xml;"
		"*.material;*.ogex;*.ms3d;*.lwo;*.lws;*.lxo;*.csm;"
		"*.cob;*.scn;*.xgl\0"
		L"3D GameStudio Model (.mdl)\0*.mdl\0"
		L"3D GameStudio Terrain (.hmp)\0*.hmp\0"
		L"3D Manufacturing Format (.3mf)\0*.3mf\0"
		L"3D Studio Max 3DS (.3ds)\0*.3ds\0"
		L"3D Studio Max ASE (.ase)\0*.ase\0"
		L"AC3D (.ac, .ac3d)\0*.ac;*.ac3d\0"
		L"AutoCAD DXF (.dxf)\0*.dxf\0"
		L"Autodesk DXF (.dxf)\0*.dxf\0"
		L"Biovision BVH (.bvh)\0*.bvh\0"
		L"Blender (.blend)\0*.blend\0"
		L"CharacterStudio Motion (.csm)\0*.csm\0"
		L"Collada (.dae, .xml)\0*.dae;*.xml\0"
		L"Doom 3 (.md5mesh, .md5anim, .md5camera)\0*.md5mesh;*.md5anim;*.md5camera\0"
		L"DirectX (.x) X\0*.x\0"
		L"FBX - Format, as ASCII and binary (.fbx)\0*.fbx\0"
		L"glTF, glTF2.0 (.glTF, .glb)\0*.glTF;*.glb\0"
		L"IFC - STEP (.ifc)\0*.ifc\0"
		L"Irrlicht Mesh (.irrmesh, .xml)\0*.irrmesh;*.xml\0"
		L"Irrlicht Scene (.irr)\0*.irr;*.xml\0"
		L"LightWave Model (.lwo)\0*.lwo\0"
		L"LightWave Scene (.lws)\0*.lws\0"
		L"Milkshape 3D (.ms3d)\0*.ms3d\0"
		L"Modo Model (.lxo)\0*.lxo\0"
		L"Neutral File Format, Sense8 WorldToolkit (.nff)\0*.nff\0"
		L"Object File Format (.off)\0*.off\0"
		L"Ogre (.mesh.xml, .skeleton.xml, .material)\0*.mesh.xml;*.skeleton.xml;*.material\0"
		L"OpenGEX - Fomat (.ogex)\0*.ogex\0"
		L"Quake 3 BSP (.pk3)\0*.pk3\0"
		L"Quake I (.mdl)\0*.mdl\0"
		L"Quake II (.md2)\0*.md2\0"
		L"Quake III (.md3)\0*.md3\0"
		L"Quick3D (.q3o, .q3s)\0*.q3o;*.q3s\0"
		L"Raw Triangles (.raw)\0*.raw\0"
		L"RtCW (.mdc)\0*.mdc\0"
		L"Stanford Ply (.ply)\0*.ply\0"
		L"Stanford Polygon Library (.ply)\0*.ply\0"
		L"Stereolithography (.stl)\0*.stl\0"
		L"Terragen Terrain (.ter)\0*.ter\0"
		L"TrueSpace (.cob, .scn)\0*.cob;*.scn\0"
		L"Valve Model (.smd, .vta)\0*.smd;*.vta\0"
		L"Wavefront Object (.obj)\0*.obj\0"
		L"XGL - 3D - Format (.xgl)\0*.xgl\0"
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

bool FileUtils::getTextureFilePath(std::string& filePath, std::string& fileName)
{
	wchar_t path[MAX_PATH] = L"";

	constexpr LPCWSTR fileFormats =
		L"All Supported Image Types (.png, .jpg)\0*.png;*.jpg\0"
		"PNG (.png, .PNG)\0*.png\0"
		"JPG (.jpg)\0*.jpg\0"
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

bool FileUtils::getLayoutFilePath(std::string& filePath, std::string& fileName)
{
	wchar_t path[MAX_PATH] = L"";

	constexpr LPCWSTR fileFormats =
		L"ImGui Layout (.ini)\0*.ini\0";

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

bool FileUtils::getSceneFilePath(std::string& filePath, std::string& fileName)
{
	wchar_t path[MAX_PATH] = L"";

	constexpr LPCWSTR fileFormats =
		L"JSON File (.json)\0*.json\0";

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
