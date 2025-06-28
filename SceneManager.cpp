///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// free up the allocated memory
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}
	// clear the collection of defined materials
	m_objectMaterials.clear();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		// find the defined material that matches the tag
		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			// pass the material properties into the shader
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);

		}
	}
}

void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/
	SceneManager::OBJECT_MATERIAL defaultMaterial;
	defaultMaterial.tag = "default";
	defaultMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.5f);
	defaultMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.8f);
	defaultMaterial.shininess = 64.0f;

	m_objectMaterials.push_back(defaultMaterial);

}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help       ***/
		// Enable custom lighting
	// Tell shaders to use lighting (this was commented out or false before)
	// SetupSceneLights()
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// ------------------------------
	// SUNSET LIGHT (Directional)
	// ------------------------------
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.5f, -1.0f, -0.3f); // angled downward
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.2f, 0.1f, 0.05f);     // subtle warm ambient
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 1.0f, 0.5f, 0.2f);      // orange glow
	m_pShaderManager->setVec3Value("directionalLight.specular", 1.0f, 0.5f, 0.3f);     // sunset shine
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// ------------------------------
	// INDOOR LIGHT (Point Light)
	// ------------------------------
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 5.0f, -8.0f);  // Inside house
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.2f, 0.15f, 0.1f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 1.0f, 0.85f, 0.6f);   // warm light
	m_pShaderManager->setVec3Value("pointLights[0].specular", 1.0f, 0.9f, 0.7f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// ------------------------------
	// PATIO LIGHT (Point Light)
	// ------------------------------
	m_pShaderManager->setVec3Value("pointLights[1].position", 0.0f, 3.0f, 2.0f);  // In front patio/table area
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.1f, 0.1f, 0.2f);   // cool blue ambiance
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.3f, 0.3f, 0.6f);   // patio lamp glow
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.5f, 0.5f, 0.9f);  // bluish edge
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	// Load scene textures from the provided files
	CreateGLTexture("Textures/Grass.jpg", "grass");
	CreateGLTexture("Textures/Sky.jpg", "sky");
	CreateGLTexture("Textures/woodseat.jpg", "woodseat");
	CreateGLTexture("Textures/woodlegs.jpg", "woodlegs");
	CreateGLTexture("Textures/roof.jpg", "roofing");
	CreateGLTexture("Textures/glass.jpg", "glass");
	CreateGLTexture("Textures/stucco.jpg", "stucco");

	// Bind all loaded textures to texture slots
	BindGLTextures();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();  // For table & chair legs
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/


void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// ===============================
	// 3D BASE PLANE - GREEN GROUND
	// ===============================
	scaleXYZ = glm::vec3(40.0f, 0.1f, 40.0f);  // Ground plane
	positionXYZ = glm::vec3(0.0f, -0.05f, 0.0f); // Slightly below origin

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.6f, 0.0f, 1.0f); // Green
	SetShaderTexture("grass");
	SetShaderMaterial("default");
	m_basicMeshes->DrawPlaneMesh();

	// ===============================
	// SKY DOME - SEMI-SPHERE INVERTED
	// ===============================
	// Simulating with large semi-cylinder for simplicity (assuming no sphere available)
	scaleXYZ = glm::vec3(50.0f, 25.0f, 50.0f); // Dome-like
	positionXYZ = glm::vec3(0.0f, 24.0f, 0.0f); // Above the ground

	SetTransformations(scaleXYZ, 180.0f, 0.0f, 0.0f, positionXYZ); // Invert to cover scene
	SetShaderColor(0.5f, 0.8f, 1.0f, 1.0f); // Sky blue
	SetShaderTexture("sky");
	SetShaderMaterial("default");
	m_basicMeshes->DrawCylinderMesh();

	// === Add your other objects like table and chairs below this ===

	// Example object (table top)
	scaleXYZ = glm::vec3(1.2f, 0.1f, 1.2f);
	positionXYZ = glm::vec3(0.0f, 1.0f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);
	SetShaderTexture("woodseat");
	SetShaderMaterial("default");
	m_basicMeshes->DrawCylinderMesh();

	// ===========================
// Render Table and Two Chairs
// ===========================

	// === Table Top (Cylinder) ===
	scaleXYZ = glm::vec3(1.2f, 0.3f, 1.2f);  // Wide and flat
	positionXYZ = glm::vec3(0.0f, 1.0f, 0.0f);  // Center of porch

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);  // Light gray
	SetShaderTexture("woodseat");
	SetShaderMaterial("default");
	m_basicMeshes->DrawCylinderMesh();

	// === Table Base (Cylinder) ===
	scaleXYZ = glm::vec3(0.2f, 0.8f, 0.2f);  // Tall and narrow
	positionXYZ = glm::vec3(0.0f, 0.4f, 0.0f);  // Under tabletop

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("woodseat");
	m_basicMeshes->DrawCylinderMesh();

	// === Left Chair Seat (Box) ===
	scaleXYZ = glm::vec3(0.6f, 0.1f, 0.6f);  // Flat seat
	positionXYZ = glm::vec3(-1.2f, 0.8f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("woodseat");
	SetShaderMaterial("default");
	m_basicMeshes->DrawBoxMesh();

	// === Left Chair Legs (4 Cylinders) ===
	scaleXYZ = glm::vec3(0.1f, 0.5f, 0.1f);
	float legY = 0.25f;
	float offsetX = 0.2f, offsetZ = 0.2f;

	glm::vec3 legPositions[4] = {
		{-1.2f - offsetX, legY,  offsetZ},  // front left
		{-1.2f + offsetX, legY,  offsetZ},  // front right
		{-1.2f - offsetX, legY, -offsetZ},  // back left
		{-1.2f + offsetX, legY, -offsetZ}   // back right
	};

	for (int i = 0; i < 4; ++i) {
		positionXYZ = legPositions[i];
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderTexture("woodseat");
		SetShaderMaterial("default");
		m_basicMeshes->DrawCylinderMesh();
	}

	// === Right Chair (Seat) ===
	scaleXYZ = glm::vec3(0.6f, 0.1f, 0.6f);
	positionXYZ = glm::vec3(1.2f, 0.8f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.4f, 0.4f, 0.4f, 1.0f);
	SetShaderTexture("woodseat");
	SetShaderMaterial("default");
	m_basicMeshes->DrawBoxMesh();

	// === Right Chair Legs (4 Cylinders) ===
	
	scaleXYZ = glm::vec3(0.1f, 0.5f, 0.1f);

	glm::vec3 rightLegPositions[4] = {
		{1.2f - offsetX, legY,  offsetZ},  // front left
		{1.2f + offsetX, legY,  offsetZ},  // front right
		{1.2f - offsetX, legY, -offsetZ},  // back left
		{1.2f + offsetX, legY, -offsetZ}   // back right
	};

	for (int i = 0; i < 4; ++i) {
		positionXYZ = rightLegPositions[i];
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

		m_basicMeshes->DrawCylinderMesh();
		SetShaderTexture("woodseat");
		SetShaderMaterial("default");
	}

	// ===============================
// MODERN HOUSE CONSTRUCTION (LARGER & PROPORTIONAL)
// ===============================

// --- Bottom Floor Base ---
	scaleXYZ = glm::vec3(8.0f, 4.0f, 10.0f);  // much larger base
	positionXYZ = glm::vec3(0.0f, 1.0f, -8.0f); // further back, raised

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

///	--- Bottom Addition ---
	scaleXYZ = glm::vec3(3.0f, 3.3f, 10.0f);  // much larger base
	positionXYZ = glm::vec3(-5.5f, 1.5f, -5.0f); // further back, raised

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
	SetShaderMaterial("default");
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

	///	--- Bottom Addition 2---
	scaleXYZ = glm::vec3(2.5f, 3.3f, 5.0f);  // much larger base
	positionXYZ = glm::vec3(5.18f, 1.5f, -6.5f); // left side

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
	SetShaderMaterial("default");
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

	///	--- Bottom Addition 3---
	scaleXYZ = glm::vec3(2.5f, 3.3f, 5.0f);  // much larger base
	positionXYZ = glm::vec3(-3.0f, 1.5f, -2.5f); // right side

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
	SetShaderMaterial("default");
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

	// --- Protuding door ---
	scaleXYZ = glm::vec3(1.5f, 3.0f, .1f); // Protruding window
	positionXYZ = glm::vec3(5.3f, 1.5f, -4.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.9f, 0.9f, 0.9f, 1.0f);
	SetShaderMaterial("default");
	SetShaderTexture("woodseat");
	m_basicMeshes->DrawBoxMesh();

	///	--- Framing Addition 1 ---
	scaleXYZ = glm::vec3(1.0f, 3.3f, 0.5f);  // Pillar
	positionXYZ = glm::vec3(-3.5f, 1.5f, 2.25f); // Perfect Positioning for frame

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
	SetShaderMaterial("default");
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

	///	--- Framing Addition 2 ---
	scaleXYZ = glm::vec3(.5f, 3.3f, 3.0f);  // thinner base for protruding right side
	positionXYZ = glm::vec3(3.5f, 1.5f, -2.5f); // Perfect Positioning for frame

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
	SetShaderMaterial("default");
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

	///	--- Framing Addition 3 ---
	scaleXYZ = glm::vec3(1.0f, 3.3f, 2.0f);  // thinner base
	positionXYZ = glm::vec3(3.5f, 1.5f, -2.5f); // Perfect Positioning for frame

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
	SetShaderMaterial("default");
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

	///	--- Framing Addition 4 ---
	scaleXYZ = glm::vec3(.5f, 4.3f, 1.0f);  // thinner base
	positionXYZ = glm::vec3(6.75f, 2.0f, -4.0f); // Perfect Positioning for frame

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
	SetShaderMaterial("default");
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

	///	--- Framing Addition 5 ---
	scaleXYZ = glm::vec3(.5f, 1.0f, 5.0f);  // 
	positionXYZ = glm::vec3(6.75f, 3.65f, -6.0f); // Perfect Positioning for frame

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
	SetShaderMaterial("default");
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

	///	--- Framing Addition 6 ---
	scaleXYZ = glm::vec3(.30f, 3.3f, 1.0f);  // Protruding section near door
	positionXYZ = glm::vec3(4.1f, 1.5f, -3.0f); // Perfect Positioning for frame

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
	SetShaderMaterial("default");
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

	// --- Top Floor Block ---
	scaleXYZ = glm::vec3(8.5f, 3.0f, 6.5f); // Main Top Floor
	positionXYZ = glm::vec3(0.0f, 4.5f, -5.75f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.9f, 0.9f, 0.9f, 1.0f);
	SetShaderMaterial("default");
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

	// --- Top Floor Block 2 ---
	scaleXYZ = glm::vec3(6.0f, 3.0f, 7.0f); // Protruding second floor
	positionXYZ = glm::vec3(-1.0f, 4.5f, -4.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.9f, 0.9f, 0.9f, 1.0f);
	SetShaderMaterial("default");
	SetShaderTexture("stucco");
	m_basicMeshes->DrawBoxMesh();

	// --- Protuding windows 1 ---
	scaleXYZ = glm::vec3(2.0f, 3.0f, .1f); // to floor Protruding window
	positionXYZ = glm::vec3(-1.8f, 4.5f, -.5f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.9f, 0.9f, 0.9f, 1.0f);
	SetShaderMaterial("default");
	SetShaderTexture("glass");
	m_basicMeshes->DrawBoxMesh();

	// --- Protuding windows 2 ---
	scaleXYZ = glm::vec3(2.0f, 3.0f, .1f); // top floor Protruding window
	positionXYZ = glm::vec3(0.2f, 4.5f, -.5f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.9f, 0.9f, 0.9f, 1.0f);
	SetShaderMaterial("default");
	SetShaderTexture("glass");
	m_basicMeshes->DrawBoxMesh();

	// --- Roof Overhang 1 ---
	scaleXYZ = glm::vec3(8.0f, 0.5f, 16.0f); // large modern roof main coverage
	positionXYZ = glm::vec3(0.0f, 2.95f, -5.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	SetShaderMaterial("default");
	SetShaderTexture("roofing");
	m_basicMeshes->DrawBoxMesh();

	// --- Roof Overhang 2 ---
	scaleXYZ = glm::vec3(8.0f, 0.5f, 12.5f); // large modern roof first floor left side
	positionXYZ = glm::vec3(-6.0f, 2.95f, -5.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	SetShaderMaterial("default");
	SetShaderTexture("roofing");
	m_basicMeshes->DrawBoxMesh();

	// --- Roof Overhang 3 ---
	scaleXYZ = glm::vec3(11.0f, 0.5f, 9.5f); // large modern roof second floor main coverage
	positionXYZ = glm::vec3(0.0f, 6.0f, -5.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	SetShaderMaterial("default");
	SetShaderTexture("roofing");
	m_basicMeshes->DrawBoxMesh();

	// --- Roof Overhang 4 ---
	scaleXYZ = glm::vec3(6.0f, 0.5f, 2.0f); // large modern roof protruding second flor
	positionXYZ = glm::vec3(-2.5f, 6.0f, 0.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	SetShaderMaterial("default");
	SetShaderTexture("roofing");
	m_basicMeshes->DrawBoxMesh();

	// --- House Floor ---
	scaleXYZ = glm::vec3(12.0f, 0.3f, 15.0f); // First floor
	positionXYZ = glm::vec3(2.0f, 0.0f, -5.0f);

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	SetShaderTexture("woodseat");
	SetShaderMaterial("default");
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

}
