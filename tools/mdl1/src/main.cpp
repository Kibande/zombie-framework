// ----------------------------------------------------------------------------
// Another Assimp OpenGL sample including texturing.
// Note that it is very basic and will only read and apply the model's diffuse
// textures (by their material ids)
//
// Don't worry about the "Couldn't load Image: ...dwarf2.jpg" Message.
// It's caused by a bad texture reference in the model file (I guess)
//
// If you intend to _use_ this code sample in your app, do yourself a favour
// and replace immediate mode calls with VBOs ...
//
// Thanks to NeHe on whose OpenGL tutorials this one's based on! :)
// http://nehe.gamedev.net/
// ----------------------------------------------------------------------------
#include <stdio.h>

#include <fstream>

//to map image filenames to textureIds
#include <string>

// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"	//OO version Header!
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/LogStream.hpp"


// the global Assimp scene object
const aiScene* scene = NULL;
aiVector3D scene_min, scene_max, scene_center;

// Create an instance of the Importer class
Assimp::Importer importer;

FILE* output;

struct ModelVertex
{
	float x, y, z;
	float u, v;
	int16_t n[4];
	uint8_t rgba[4];
};

void createAILogger()
{
    // Change this line to normal if you not want to analyse the import process
	//Assimp::Logger::LogSeverity severity = Assimp::Logger::NORMAL;
	Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;

	// Create a logger instance for Console Output
	Assimp::DefaultLogger::create("",severity, aiDefaultLogStream_STDERR);

	// Create a logger instance for File Output (found in project folder or near .exe)
	Assimp::DefaultLogger::create("assimp_log.txt",severity, aiDefaultLogStream_FILE);

	// Now I am ready for logging my stuff
	Assimp::DefaultLogger::get()->info("this is my info-call");
}

void destroyAILogger()
{
	// Kill it after the work is done
	Assimp::DefaultLogger::kill();
}

void logInfo(std::string logString)
{
	// Will add message to File with "info" Tag
	Assimp::DefaultLogger::get()->info(logString.c_str());
}

void logDebug(const char* logString)
{
	// Will add message to File with "debug" Tag
	Assimp::DefaultLogger::get()->debug(logString);
}


bool Import3DFromFile( const std::string& pFile)
{
	// Check if file exists
	std::ifstream fin(pFile.c_str());
	if(!fin.fail())
	{
		fin.close();
	}
	else
	{
		logInfo( importer.GetErrorString());
		return false;
	}

	scene = importer.ReadFile(pFile,
			aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Quality);

	// If the import failed, report it
	if( !scene)
	{
		logInfo( importer.GetErrorString());
		return false;
	}

	// Now we can access the file's contents.
	logInfo("Import of scene " + pFile + " succeeded.");

	// We're done. Everything will be cleaned up by the importer destructor
	return true;
}

std::string getBasePath(const std::string& path)
{
	size_t pos = path.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : path.substr(0, pos + 1);
}


void set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

void color4_to_float4(const aiColor4D *c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

void recursive_render (const struct aiScene *sc, const struct aiNode* nd, aiMatrix4x4 transform)
{
	unsigned int i;
	unsigned int n=0, t;
	aiMatrix4x4 m = nd->mTransformation;

	transform = m * transform;

	//fprintf(stderr, "ENTER %s\n", nd->mName.C_Str());

	// draw all meshes assigned to this node
	for (; n < nd->mNumMeshes; ++n)
	{
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

		for (t = 0; t < mesh->mNumFaces; ++t) {
			const struct aiFace* face = &mesh->mFaces[t];

			if (face->mNumIndices != 3)
				fprintf(stderr, "Error: skipping invalid face %d - %d vertices!\n", t, face->mNumIndices);

			//fprintf(stderr, "FACE %d:\n", t);

			ModelVertex vertices[3];

			for (i = 0; i < face->mNumIndices; i++)
			{
				int vertexIndex = face->mIndices[i];

				if (mesh->mColors[0] != NULL)
				{
					vertices[i].rgba[0] = (uint8_t)(mesh->mColors[0][vertexIndex].r * 255.0f);
					vertices[i].rgba[1] = (uint8_t)(mesh->mColors[0][vertexIndex].g * 255.0f);
					vertices[i].rgba[2] = (uint8_t)(mesh->mColors[0][vertexIndex].b * 255.0f);
					vertices[i].rgba[3] = (uint8_t)(mesh->mColors[0][vertexIndex].a * 255.0f);
				}
				else
				{
					vertices[i].rgba[0] = 0xff;
					vertices[i].rgba[1] = 0xff;
					vertices[i].rgba[2] = 0xff;
					vertices[i].rgba[3] = 0xff;
				}

				if (mesh->HasNormals())
				{
					vertices[i].n[0] = (int16_t)(mesh->mNormals[vertexIndex].x * INT16_MAX);
					vertices[i].n[1] = -(int16_t)(mesh->mNormals[vertexIndex].y * INT16_MAX);
					vertices[i].n[2] = -(int16_t)(mesh->mNormals[vertexIndex].z * INT16_MAX);
					vertices[i].n[3] = 0;
				}
				else
				{
					vertices[i].n[0] = 0;
					vertices[i].n[1] = 0;
					vertices[i].n[2] = INT16_MAX;
					vertices[i].n[3] = 0;
				}

				if (mesh->HasTextureCoords(0))
				{
					vertices[i].u = mesh->mTextureCoords[0][vertexIndex].x;
					vertices[i].v = 1.0f - mesh->mTextureCoords[0][vertexIndex].y;
				}
				else
				{
					vertices[i].u = 0.0f;
					vertices[i].v = 0.0f;
				}

				aiVector3D vtx = m * mesh->mVertices[vertexIndex];

				vertices[i].x = vtx.x;
				vertices[i].y = -vtx.y;
				vertices[i].z = -vtx.z;
			}

			fwrite(&vertices, sizeof(vertices), 1, output);
		}
	}

	// draw all children
	for (n = 0; n < nd->mNumChildren; ++n)
	{
		recursive_render(sc, nd->mChildren[n], m);
	}

	//fprintf(stderr, "LEAVE %s\n", nd->mName.C_Str());
}


void drawAiScene(const aiScene* scene)
{
	logInfo("drawing objects");

	aiMatrix4x4 m;
	recursive_render(scene, scene->mRootNode, m);
}

int main(int argc, char** argv)
{
	createAILogger();
	logInfo("App fired!");

	const char* input = argv[1];
	auto outName = std::string(getenv("TMP")) + tmpnam(NULL);

	output = fopen(outName.c_str(), "wb");

	if (!Import3DFromFile(argv[1])) return 0;

	logInfo("=============== Post Import ====================");
	drawAiScene(scene);

	fclose(output);

	printf("mdl1\t%s\n", outName.c_str());

	return 0;
}
