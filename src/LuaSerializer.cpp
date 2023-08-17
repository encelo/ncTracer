#include "LuaSerializer.h"
#include <nctl/String.h>
#include <cstring>
#include <ncine/IFile.h>
#include <ncine/LuaStateManager.h>
#include <ncine/LuaUtils.h>

#include "World.h"

#include "Plane.h"
#include "Sphere.h"
#include "Rectangle.h"

#include "Matte.h"
#include "Phong.h"
#include "Emissive.h"

#include "Directional.h"
#include "PointLight.h"
#include "Ambient.h"
#include "AmbientOccluder.h"
#include "AreaLight.h"
#include "EnvironmentLight.h"

#include "Regular.h"
#include "PureRandom.h"
#include "Jittered.h"
#include "MultiJittered.h"
#include "NRooks.h"
#include "Hammersley.h"
#include "Halton.h"

namespace nc = ncine;

namespace {

nctl::String &indent(nctl::String &string, int amount)
{
	FATAL_ASSERT(amount >= 0);
	for (int i = 0; i < amount; i++)
		string.append("\t");
	return string;
}

int samplerIndex(const pm::World &world, const pm::Sampler *sampler)
{
	ASSERT(sampler);

	int index = -1;
	for (unsigned int i = 0; i < world.samplers().size(); i++)
	{
		if (sampler == world.samplers()[i].get())
		{
			index = static_cast<int>(i);
			break;
		}
	}

	return index;
}

int materialIndex(const pm::World &world, const pm::Material *material)
{
	ASSERT(material);

	int index = -1;
	for (unsigned int i = 0; i < world.materials().size(); i++)
	{
		if (material == world.materials()[i].get())
		{
			index = static_cast<int>(i);
			break;
		}
	}

	return index;
}

int geometryIndex(const pm::World &world, const pm::Geometry *geometry)
{
	ASSERT(geometry);

	int index = -1;
	for (unsigned int i = 0; i < world.objects().size(); i++)
	{
		if (geometry == world.objects()[i].get())
		{
			index = static_cast<int>(i);
			break;
		}
	}

	return index;
}

const char *samplerTypeToString(pm::Sampler::Type type)
{
	switch (type)
	{
		case pm::Sampler::Type::REGULAR: return "regular";
		case pm::Sampler::Type::PURE_RANDOM: return "pure_random";
		case pm::Sampler::Type::JITTERED: return "jittered";
		case pm::Sampler::Type::MULTI_JITTERED: return "multi_jittered";
		case pm::Sampler::Type::NROOKS: return "nrooks";
		case pm::Sampler::Type::HAMMERSLEY: return "hammersley";
		case pm::Sampler::Type::HALTON: return "halton";
	}
	return "unknown";
}

const char *materialTypeToString(pm::Material::Type type)
{
	switch (type)
	{
		case pm::Material::Type::MATTE: return "matte";
		case pm::Material::Type::PHONG: return "phong";
		case pm::Material::Type::EMISSIVE: return "emissive";
	}
	return "unknown";
}

const char *geometryTypeToString(pm::Geometry::Type type)
{
	switch (type)
	{
		case pm::Geometry::Type::PLANE: return "plane";
		case pm::Geometry::Type::SPHERE: return "sphere";
		case pm::Geometry::Type::RECTANGLE: return "rectangle";
	}
	return "unknown";
}

const char *lightTypeToString(pm::Light::Type type)
{
	switch (type)
	{
		case pm::Light::Type::DIRECTIONAL: return "directional";
		case pm::Light::Type::POINT: return "point";
		case pm::Light::Type::AMBIENT: return "ambient";
		case pm::Light::Type::AMBIENT_OCCLUDER: return "ambient_occluder";
		case pm::Light::Type::AREA: return "area";
		case pm::Light::Type::ENVIRONMENT: return "environment";
	}
	return "unknown";
}

pm::Sampler::Type samplerStringToType(const char *type)
{
	ASSERT(type);

	if (strncmp(type, samplerTypeToString(pm::Sampler::Type::REGULAR), 32) == 0)
		return pm::Sampler::Type::REGULAR;
	else if (strncmp(type, samplerTypeToString(pm::Sampler::Type::PURE_RANDOM), 32) == 0)
		return pm::Sampler::Type::PURE_RANDOM;
	else if (strncmp(type, samplerTypeToString(pm::Sampler::Type::JITTERED), 32) == 0)
		return pm::Sampler::Type::JITTERED;
	else if (strncmp(type, samplerTypeToString(pm::Sampler::Type::MULTI_JITTERED), 32) == 0)
		return pm::Sampler::Type::MULTI_JITTERED;
	else if (strncmp(type, samplerTypeToString(pm::Sampler::Type::NROOKS), 32) == 0)
		return pm::Sampler::Type::NROOKS;
	else if (strncmp(type, samplerTypeToString(pm::Sampler::Type::HAMMERSLEY), 32) == 0)
		return pm::Sampler::Type::HAMMERSLEY;
	else if (strncmp(type, samplerTypeToString(pm::Sampler::Type::HALTON), 32) == 0)
		return pm::Sampler::Type::HALTON;

	return pm::Sampler::Type::REGULAR;
}

pm::Material::Type materialStringToType(const char *type)
{
	ASSERT(type);

	if (strncmp(type, materialTypeToString(pm::Material::Type::MATTE), 32) == 0)
		return pm::Material::Type::MATTE;
	else if (strncmp(type, materialTypeToString(pm::Material::Type::PHONG), 32) == 0)
		return pm::Material::Type::PHONG;
	else if (strncmp(type, materialTypeToString(pm::Material::Type::EMISSIVE), 32) == 0)
		return pm::Material::Type::EMISSIVE;

	return pm::Material::Type::MATTE;
}

pm::Geometry::Type geometryStringToType(const char *type)
{
	ASSERT(type);

	if (strncmp(type, geometryTypeToString(pm::Geometry::Type::PLANE), 32) == 0)
		return pm::Geometry::Type::PLANE;
	else if (strncmp(type, geometryTypeToString(pm::Geometry::Type::SPHERE), 32) == 0)
		return pm::Geometry::Type::SPHERE;
	else if (strncmp(type, geometryTypeToString(pm::Geometry::Type::RECTANGLE), 32) == 0)
		return pm::Geometry::Type::RECTANGLE;

	return pm::Geometry::Type::PLANE;
}

pm::Light::Type lightStringToType(const char *type)
{
	ASSERT(type);

	if (strncmp(type, lightTypeToString(pm::Light::Type::DIRECTIONAL), 32) == 0)
		return pm::Light::Type::DIRECTIONAL;
	else if (strncmp(type, lightTypeToString(pm::Light::Type::POINT), 32) == 0)
		return pm::Light::Type::POINT;
	else if (strncmp(type, lightTypeToString(pm::Light::Type::AMBIENT), 32) == 0)
		return pm::Light::Type::AMBIENT;
	else if (strncmp(type, lightTypeToString(pm::Light::Type::AMBIENT_OCCLUDER), 32) == 0)
		return pm::Light::Type::AMBIENT_OCCLUDER;
	else if (strncmp(type, lightTypeToString(pm::Light::Type::AREA), 32) == 0)
		return pm::Light::Type::AREA;
	else if (strncmp(type, lightTypeToString(pm::Light::Type::ENVIRONMENT), 32) == 0)
		return pm::Light::Type::ENVIRONMENT;

	return pm::Light::Type::DIRECTIONAL;
}

pm::RGBColor retrieveLuaColor(lua_State *L, int index)
{
	const float red = nc::LuaUtils::retrieveField<float>(L, index, "r");
	const float green = nc::LuaUtils::retrieveField<float>(L, index, "g");
	const float blue = nc::LuaUtils::retrieveField<float>(L, index, "b");
	return pm::RGBColor(red, green, blue);
}

pm::RGBColor retrieveLuaColorFieldTable(lua_State *L, int index, const char *name)
{

	nc::LuaUtils::retrieveFieldTable(L, index, name);
	const pm::RGBColor color = retrieveLuaColor(L, -1);
	nc::LuaUtils::pop(L);
	return color;
}

pm::Vector3 retrieveLuaVector3(lua_State *L, int index)
{
	const float x = nc::LuaUtils::retrieveField<float>(L, index, "x");
	const float y = nc::LuaUtils::retrieveField<float>(L, index, "y");
	const float z = nc::LuaUtils::retrieveField<float>(L, index, "z");
	return pm::Vector3(x, y, z);
}

pm::Vector3 retrieveLuaVector3FieldTable(lua_State *L, int index, const char *name)
{

	nc::LuaUtils::retrieveFieldTable(L, index, name);
	const pm::Vector3 vector = retrieveLuaVector3(L, -1);
	nc::LuaUtils::pop(L);
	return vector;
}

const unsigned int ProjectFileVersion = 1;

namespace Names {

	const char *version = "project_version";

	const char *world = "world";
	const char *backgroundColor = "background_color";
	const char *type = "type";

	const char *samplers = "samplers";
	const char *numSamples = "num_samples";

	const char *viewplane = "viewplane";
	const char *width = "width";
	const char *height = "height";
	const char *pixelSize = "pixelSize";
	const char *gamma = "gamma";
	const char *maxDepth = "max_depth";

	const char *materials = "materials";
	const char *ambientKd = "ambient_kd";
	const char *ambientCd = "ambient_cd";
	const char *ambientSamplerIndex = "ambient_sampler_index";
	const char *diffuseKd = "diffuse_kd";
	const char *diffuseCd = "diffuse_cd";
	const char *diffuseSamplerIndex = "diffuse_sampler_index";
	const char *specularKs = "specular_ks";
	const char *specularCs = "specular_cs";
	const char *specularExp = "specular_exp";
	const char *specularSamplerIndex = "specular_sampler_iindex";
	const char *radianceScale = "radiance_scale";
	const char *emissiveCe = "emissive_ce";

	const char *geometries = "geometries";
	const char *materialIndex = "material_index";
	const char *point = "point";
	const char *normal = "normal";
	const char *center = "center";
	const char *radius = "radius";
	const char *sideA = "side_a";
	const char *sideB = "side_b";

	const char *lights = "lights";
	const char *castShadows = "cast_shadows";
	const char *color = "color";
	const char *direction = "direction";
	const char *location = "location";
	const char *minAmount = "min_amount";
	const char *samplerIndex = "sampler_index";
	const char *objectIndex = "object_index";
}

}

///////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
///////////////////////////////////////////////////////////

bool LuaSerializer::load(const char *filename, pm::World &world)
{
	nc::LuaStateManager luaState =
	    nc::LuaStateManager(nc::LuaStateManager::ApiType::NONE,
	                        nc::LuaStateManager::StatisticsTracking::DISABLED,
	                        nc::LuaStateManager::StandardLibraries::NOT_LOADED);

	if (luaState.runFromFile(filename) == false)
		return false;
	lua_State *L = luaState.state();

	unsigned int version = 1;
	nc::LuaUtils::tryRetrieveGlobal<uint32_t>(L, Names::version, version);
	ASSERT(version = ProjectFileVersion);

	// World has to be cleared for sampler/geometry/material indices to work
	world.clear();

	nc::LuaUtils::retrieveGlobalTable(L, Names::world);

	world.editBackground() = retrieveLuaColorFieldTable(L, -1, Names::backgroundColor);

	// Samplers loader
	nc::LuaUtils::retrieveFieldTable(L, -1, Names::samplers);
	const unsigned int numSamplers = nc::LuaUtils::rawLen(L, -1);

	for (unsigned int i = 0; i < numSamplers; i++)
	{
		nc::LuaUtils::rawGeti(L, -1, i + 1); // Lua arrays start from index 1

		const char *typeString = nc::LuaUtils::retrieveField<const char *>(L, -1, Names::type);
		const unsigned int numSamples = nc::LuaUtils::retrieveField<uint32_t>(L, -1, Names::numSamples);

		const pm::Sampler::Type type = samplerStringToType(typeString);
		std::unique_ptr<pm::Sampler> sampler;
		switch (type)
		{
			case pm::Sampler::Type::REGULAR:
				sampler = std::make_unique<pm::Regular>(numSamples);
				break;
			case pm::Sampler::Type::PURE_RANDOM:
				sampler = std::make_unique<pm::PureRandom>(numSamples);
				break;
			case pm::Sampler::Type::JITTERED:
				sampler = std::make_unique<pm::Jittered>(numSamples);
				break;
			case pm::Sampler::Type::MULTI_JITTERED:
				sampler = std::make_unique<pm::MultiJittered>(numSamples);
				break;
			case pm::Sampler::Type::NROOKS:
				sampler = std::make_unique<pm::NRooks>(numSamples);
				break;
			case pm::Sampler::Type::HAMMERSLEY:
				sampler = std::make_unique<pm::Hammersley>(numSamples);
				break;
			case pm::Sampler::Type::HALTON:
				sampler = std::make_unique<pm::Halton>(numSamples);
				break;
		}
		world.addSampler(std::move(sampler));

		nc::LuaUtils::pop(L);
	}
	nc::LuaUtils::pop(L);

	// Viewplane loader
	nc::LuaUtils::retrieveFieldTable(L, -1, Names::viewplane);

	world.viewPlane().editWidth() = nc::LuaUtils::retrieveField<int32_t>(L, -1, Names::width);
	world.viewPlane().editHeight() = nc::LuaUtils::retrieveField<int32_t>(L, -1, Names::height);
	world.viewPlane().editPixelSize() = nc::LuaUtils::retrieveField<float>(L, -1, Names::pixelSize);
	const float gamma = nc::LuaUtils::retrieveField<float>(L, -1, Names::gamma);
	world.viewPlane().setGamma(gamma);
	world.viewPlane().editMaxDepth() = nc::LuaUtils::retrieveField<int32_t>(L, -1, Names::maxDepth);
	const unsigned int samplerIndex = nc::LuaUtils::retrieveField<uint32_t>(L, -1, Names::samplerIndex);
	world.viewPlane().setSampler(world.samplers()[samplerIndex].get());

	nc::LuaUtils::pop(L);

	// Materials loader
	nc::LuaUtils::retrieveFieldTable(L, -1, Names::materials);
	const unsigned int numMaterials = nc::LuaUtils::rawLen(L, -1);

	for (unsigned int i = 0; i < numMaterials; i++)
	{
		nc::LuaUtils::rawGeti(L, -1, i + 1); // Lua arrays start from index 1

		const char *typeString = nc::LuaUtils::retrieveField<const char *>(L, -1, Names::type);

		const pm::Material::Type type = materialStringToType(typeString);
		switch (type)
		{
			case pm::Material::Type::MATTE:
			{
				std::unique_ptr<pm::Matte> matte = std::make_unique<pm::Matte>();

				matte->ambient().editKd() = nc::LuaUtils::retrieveField<float>(L, -1, Names::ambientKd);
				matte->ambient().editCd() = retrieveLuaColorFieldTable(L, -1, Names::ambientCd);
				const unsigned int ambientSamplerIndex = nc::LuaUtils::retrieveField<uint32_t>(L, -1, Names::ambientSamplerIndex);
				matte->ambient().setSampler(world.samplers()[ambientSamplerIndex].get());

				matte->diffuse().editKd() = nc::LuaUtils::retrieveField<float>(L, -1, Names::diffuseKd);
				matte->diffuse().editCd() = retrieveLuaColorFieldTable(L, -1, Names::diffuseCd);
				const unsigned int diffuseSamplerIndex = nc::LuaUtils::retrieveField<uint32_t>(L, -1, Names::diffuseSamplerIndex);
				matte->diffuse().setSampler(world.samplers()[diffuseSamplerIndex].get());

				world.addMaterial(std::move(matte));
				break;
			}
			case pm::Material::Type::PHONG:
			{
				std::unique_ptr<pm::Phong> phong = std::make_unique<pm::Phong>();

				phong->ambient().editKd() = nc::LuaUtils::retrieveField<float>(L, -1, Names::ambientKd);
				phong->ambient().editCd() = retrieveLuaColorFieldTable(L, -1, Names::ambientCd);
				const unsigned int ambientSamplerIndex = nc::LuaUtils::retrieveField<uint32_t>(L, -1, Names::ambientSamplerIndex);
				phong->ambient().setSampler(world.samplers()[ambientSamplerIndex].get());

				phong->diffuse().editKd() = nc::LuaUtils::retrieveField<float>(L, -1, Names::diffuseKd);
				phong->diffuse().editCd() = retrieveLuaColorFieldTable(L, -1, Names::diffuseCd);
				const unsigned int diffuseSamplerIndex = nc::LuaUtils::retrieveField<uint32_t>(L, -1, Names::diffuseSamplerIndex);
				phong->diffuse().setSampler(world.samplers()[diffuseSamplerIndex].get());

				phong->specular().editKs() = nc::LuaUtils::retrieveField<float>(L, -1, Names::specularKs);
				phong->specular().editCs() = retrieveLuaColorFieldTable(L, -1, Names::specularCs);
				phong->specular().editExp() = nc::LuaUtils::retrieveField<float>(L, -1, Names::specularExp);
				const unsigned int specularSamplerIndex = nc::LuaUtils::retrieveField<uint32_t>(L, -1, Names::specularSamplerIndex);
				phong->specular().setSampler(world.samplers()[specularSamplerIndex].get());

				world.addMaterial(std::move(phong));
				break;
			}
			case pm::Material::Type::EMISSIVE:
			{
				std::unique_ptr<pm::Emissive> emissive = std::make_unique<pm::Emissive>();

				emissive->editRadianceScale() = nc::LuaUtils::retrieveField<float>(L, -1, Names::radianceScale);
				emissive->editCe() = retrieveLuaColorFieldTable(L, -1, Names::emissiveCe);

				world.addMaterial(std::move(emissive));
				break;
			}
		}

		nc::LuaUtils::pop(L);
	}
	nc::LuaUtils::pop(L);

	// Geometries loader
	nc::LuaUtils::retrieveFieldTable(L, -1, Names::geometries);
	const unsigned int numGeometries = nc::LuaUtils::rawLen(L, -1);

	for (unsigned int i = 0; i < numGeometries; i++)
	{
		nc::LuaUtils::rawGeti(L, -1, i + 1); // Lua arrays start from index 1

		const char *typeString = nc::LuaUtils::retrieveField<const char *>(L, -1, Names::type);
		const bool castShadows = nc::LuaUtils::retrieveField<bool>(L, -1, Names::castShadows);
		const unsigned int materialIndex = nc::LuaUtils::retrieveField<uint32_t>(L, -1, Names::materialIndex);

		const pm::Geometry::Type type = geometryStringToType(typeString);
		switch (type)
		{
			case pm::Geometry::Type::PLANE:
			{
				std::unique_ptr<pm::Plane> plane = std::make_unique<pm::Plane>();

				plane->editCastShadows() = castShadows;
				plane->setMaterial(world.materials()[materialIndex].get());
				plane->editPoint() = retrieveLuaVector3FieldTable(L, -1, Names::point);
				plane->editNormal() = retrieveLuaVector3FieldTable(L, -1, Names::normal);

				world.addObject(std::move(plane));
				break;
			}
			case pm::Geometry::Type::SPHERE:
			{
				std::unique_ptr<pm::Sphere> sphere = std::make_unique<pm::Sphere>();

				sphere->editCastShadows() = castShadows;
				sphere->setMaterial(world.materials()[materialIndex].get());
				sphere->editCenter() = retrieveLuaVector3FieldTable(L, -1, Names::center);
				sphere->editRadius() = nc::LuaUtils::retrieveField<float>(L, -1, Names::radius);

				world.addObject(std::move(sphere));
				break;
			}
			case pm::Geometry::Type::RECTANGLE:
			{
				std::unique_ptr<pm::Rectangle> rectangle = std::make_unique<pm::Rectangle>();

				rectangle->editCastShadows() = castShadows;
				rectangle->setMaterial(world.materials()[materialIndex].get());
				rectangle->editPoint() = retrieveLuaVector3FieldTable(L, -1, Names::point);
				rectangle->editSideA() = retrieveLuaVector3FieldTable(L, -1, Names::sideA);
				rectangle->editSideB() = retrieveLuaVector3FieldTable(L, -1, Names::sideB);
				rectangle->editNormal() = retrieveLuaVector3FieldTable(L, -1, Names::normal);
				rectangle->updateDimensions();

				world.addObject(std::move(rectangle));
				break;
			}
		}

		nc::LuaUtils::pop(L);
	}
	nc::LuaUtils::pop(L);

	// Lights loader
	nc::LuaUtils::retrieveFieldTable(L, -1, Names::lights);
	const unsigned int numLights = nc::LuaUtils::rawLen(L, -1);

	for (unsigned int i = 0; i < numLights; i++)
	{
		nc::LuaUtils::rawGeti(L, -1, i + 1); // Lua arrays start from index 1

		const char *typeString = nc::LuaUtils::retrieveField<const char *>(L, -1, Names::type);
		const bool castShadows = nc::LuaUtils::retrieveField<bool>(L, -1, Names::castShadows);

		const pm::Light::Type type = lightStringToType(typeString);
		switch (type)
		{
			case pm::Light::Type::DIRECTIONAL:
			{
				std::unique_ptr<pm::Directional> directional = std::make_unique<pm::Directional>();

				directional->editCastShadows() = castShadows;
				directional->editRadianceScale() = nc::LuaUtils::retrieveField<float>(L, -1, Names::radianceScale);
				directional->editColor() = retrieveLuaColorFieldTable(L, -1, Names::color);
				directional->editDirection() = retrieveLuaVector3FieldTable(L, -1, Names::direction);

				world.addLight(std::move(directional));
				break;
			}
			case pm::Light::Type::POINT:
			{
				std::unique_ptr<pm::PointLight> pointLight = std::make_unique<pm::PointLight>();

				pointLight->editCastShadows() = castShadows;
				pointLight->editRadianceScale() = nc::LuaUtils::retrieveField<float>(L, -1, Names::radianceScale);
				pointLight->editColor() = retrieveLuaColorFieldTable(L, -1, Names::color);
				pointLight->editLocation() = retrieveLuaVector3FieldTable(L, -1, Names::location);

				world.addLight(std::move(pointLight));
				break;
			}
			case pm::Light::Type::AMBIENT:
			{
				std::unique_ptr<pm::Ambient> ambient = std::make_unique<pm::Ambient>();

				ambient->editCastShadows() = castShadows;
				ambient->editRadianceScale() = nc::LuaUtils::retrieveField<float>(L, -1, Names::radianceScale);
				ambient->editColor() = retrieveLuaColorFieldTable(L, -1, Names::color);

				world.addLight(std::move(ambient));
				break;
			}
			case pm::Light::Type::AMBIENT_OCCLUDER:
			{
				std::unique_ptr<pm::AmbientOccluder> ambientOccluder = std::make_unique<pm::AmbientOccluder>();

				ambientOccluder->editCastShadows() = castShadows;
				ambientOccluder->editRadianceScale() = nc::LuaUtils::retrieveField<float>(L, -1, Names::radianceScale);
				ambientOccluder->editColor() = retrieveLuaColorFieldTable(L, -1, Names::color);
				ambientOccluder->editMinAmount() = retrieveLuaColorFieldTable(L, -1, Names::minAmount);
				const unsigned int samplerIndex = nc::LuaUtils::retrieveField<uint32_t>(L, -1, Names::diffuseSamplerIndex);
				ambientOccluder->setSampler(world.samplers()[samplerIndex].get());

				world.addLight(std::move(ambientOccluder));
				break;
			}
			case pm::Light::Type::AREA:
			{
				const unsigned int objectIndex = nc::LuaUtils::retrieveField<uint32_t>(L, -1, Names::objectIndex);
				pm::Geometry *geometry = world.objects()[objectIndex].get();
				std::unique_ptr<pm::AreaLight> areaLight = std::make_unique<pm::AreaLight>(geometry);
				areaLight->editCastShadows() = castShadows;

				world.addLight(std::move(areaLight));
				break;
			}
			case pm::Light::Type::ENVIRONMENT:
			{
				const unsigned int materialIndex = nc::LuaUtils::retrieveField<uint32_t>(L, -1, Names::materialIndex);
				pm::Material *material = world.materials()[materialIndex].get();
				ASSERT(material->type() == pm::Material::Type::EMISSIVE);
				pm::Emissive *emissive = static_cast<pm::Emissive *>(material);
				std::unique_ptr<pm::EnvironmentLight> environmentLight = std::make_unique<pm::EnvironmentLight>(emissive);
				environmentLight->editCastShadows() = castShadows;

				world.addLight(std::move(environmentLight));
				break;
			}
		}

		nc::LuaUtils::pop(L);
	}
	nc::LuaUtils::pop(L);

	return true;
}

void LuaSerializer::save(const char *filename, const pm::World &world)
{
	nctl::String file(64 * 1024); // TODO: hard-coded
	int amount = 0;

	indent(file, amount).formatAppend("%s = %u\n", Names::version, ProjectFileVersion);

	indent(file, amount).formatAppend("%s = \n", Names::world);
	indent(file, amount).append("{\n");
	amount++;

	const pm::RGBColor &bgColor = world.background();
	indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f},\n", Names::backgroundColor, bgColor.r, bgColor.g, bgColor.b);
	file.append("\n");

	// Samplers serializer
	indent(file, amount).formatAppend("%s =\n", Names::samplers);
	indent(file, amount).append("{\n");
	amount++;
	for (unsigned int i = 0; i < world.samplers().size(); i++)
	{
		indent(file, amount).append("{\n");

		amount++;
		const pm::Sampler *sampler = world.samplers()[i].get();
		const pm::Sampler::Type samplerType = sampler->type();
		indent(file, amount).formatAppend("%s = \"%s\",\n", Names::type, samplerTypeToString(samplerType));
		indent(file, amount).formatAppend("%s = %d\n", Names::numSamples, sampler->numSamples());

		const bool isLastSampler = (i == world.samplers().size() - 1);
		amount--;
		indent(file, amount).formatAppend("}%s", isLastSampler ? "" : ",\n");
		file.append("\n");
	}
	amount--;
	indent(file, amount).append("},\n");
	file.append("\n");

	// Viewplane serializer
	indent(file, amount).formatAppend("%s =\n", Names::viewplane);
	indent(file, amount).append("{\n");
	amount++;

	indent(file, amount).formatAppend("%s = %d,\n", Names::width, world.viewPlane().width());
	indent(file, amount).formatAppend("%s = %d,\n", Names::height, world.viewPlane().height());
	indent(file, amount).formatAppend("%s = %f,\n", Names::pixelSize, world.viewPlane().pixelSize());
	indent(file, amount).formatAppend("%s = %f,\n", Names::gamma, world.viewPlane().gamma());
	indent(file, amount).formatAppend("%s = %d,\n", Names::maxDepth, world.viewPlane().maxDepth());
	indent(file, amount).formatAppend("%s = %d\n", Names::samplerIndex, samplerIndex(world, world.viewPlane().sampler()));

	amount--;
	indent(file, amount).append("},\n");
	file.append("\n");

	// Materials serializer
	indent(file, amount).formatAppend("%s =\n", Names::materials);
	indent(file, amount).append("{\n");
	amount++;
	for (unsigned int i = 0; i < world.materials().size(); i++)
	{
		indent(file, amount).append("{\n");

		amount++;
		const pm::Material *material = world.materials()[i].get();
		const pm::Material::Type materialType = material->type();
		indent(file, amount).formatAppend("%s = \"%s\",\n", Names::type, materialTypeToString(materialType));

		switch (materialType)
		{
			case pm::Material::Type::MATTE:
			{
				const pm::Matte *matte = static_cast<const pm::Matte *>(material);
				indent(file, amount).formatAppend("%s = %f,\n", Names::ambientKd, matte->ambient().kd());
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f},\n", Names::ambientCd, matte->ambient().cd().r, matte->ambient().cd().g, matte->ambient().cd().b);
				indent(file, amount).formatAppend("%s = %d,\n", Names::ambientSamplerIndex, samplerIndex(world, matte->ambient().sampler()));
				indent(file, amount).formatAppend("%s = %f,\n", Names::diffuseKd, matte->diffuse().kd());
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f},\n", Names::diffuseCd, matte->diffuse().cd().r, matte->diffuse().cd().g, matte->diffuse().cd().b);
				indent(file, amount).formatAppend("%s = %d\n", Names::diffuseSamplerIndex, samplerIndex(world, matte->diffuse().sampler()));
				break;
			}
			case pm::Material::Type::PHONG:
			{
				const pm::Phong *phong = static_cast<const pm::Phong *>(material);
				indent(file, amount).formatAppend("%s = %f,\n", Names::ambientKd, phong->ambient().kd());
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f},\n", Names::ambientCd, phong->ambient().cd().r, phong->ambient().cd().g, phong->ambient().cd().b);
				indent(file, amount).formatAppend("%s = %d,\n", Names::ambientSamplerIndex, samplerIndex(world, phong->ambient().sampler()));
				indent(file, amount).formatAppend("%s = %f,\n", Names::diffuseKd, phong->diffuse().kd());
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f},\n", Names::diffuseCd, phong->diffuse().cd().r, phong->diffuse().cd().g, phong->diffuse().cd().b);
				indent(file, amount).formatAppend("%s = %d,\n", Names::diffuseSamplerIndex, samplerIndex(world, phong->diffuse().sampler()));
				indent(file, amount).formatAppend("%s = %f,\n", Names::specularKs, phong->specular().ks());
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f},\n", Names::specularCs, phong->specular().cs().r, phong->specular().cs().g, phong->specular().cs().b);
				indent(file, amount).formatAppend("%s = %f,\n", Names::specularExp, phong->specular().exp());
				indent(file, amount).formatAppend("%s = %d\n", Names::specularSamplerIndex, samplerIndex(world, phong->specular().sampler()));
				break;
			}
			case pm::Material::Type::EMISSIVE:
			{
				const pm::Emissive *emissive = static_cast<const pm::Emissive *>(material);
				indent(file, amount).formatAppend("%s = %f,\n", Names::radianceScale, emissive->radianceScale());
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f}\n", Names::emissiveCe, emissive->ce().r, emissive->ce().g, emissive->ce().b);
				break;
			}
		}

		const bool isLastMaterial = (i == world.materials().size() - 1);
		amount--;
		indent(file, amount).formatAppend("}%s", isLastMaterial ? "" : ",\n");
		file.append("\n");
	}
	amount--;
	indent(file, amount).append("},\n");
	file.append("\n");

	// Geometries serializer
	indent(file, amount).formatAppend("%s =\n", Names::geometries);
	indent(file, amount).append("{\n");
	amount++;
	for (unsigned int i = 0; i < world.objects().size(); i++)
	{
		indent(file, amount).append("{\n");

		amount++;
		const pm::Geometry *geometry = world.objects()[i].get();
		const pm::Geometry::Type geometryType = geometry->type();
		indent(file, amount).formatAppend("%s = \"%s\",\n", Names::type, geometryTypeToString(geometryType));
		indent(file, amount).formatAppend("%s = %s,\n", Names::castShadows, geometry->castShadows() ? "true" : "false");
		indent(file, amount).formatAppend("%s = %d,\n", Names::materialIndex, materialIndex(world, geometry->material()));

		switch (geometryType)
		{
			case pm::Geometry::Type::PLANE:
			{
				const pm::Plane *plane = static_cast<const pm::Plane *>(geometry);
				indent(file, amount).formatAppend("%s = {x = %f, y = %f, z = %f},\n", Names::point, plane->point().x, plane->point().y, plane->point().z);
				indent(file, amount).formatAppend("%s = {x = %f, y = %f, z = %f}\n", Names::normal, plane->normal().x, plane->normal().y, plane->normal().z);
				break;
			}
			case pm::Geometry::Type::SPHERE:
			{
				const pm::Sphere *sphere = static_cast<const pm::Sphere *>(geometry);
				indent(file, amount).formatAppend("%s = {x = %f, y = %f, z = %f},\n", Names::center, sphere->center().x, sphere->center().y, sphere->center().z);
				indent(file, amount).formatAppend("%s = %f}\n", Names::radius, sphere->radius());
				break;
			}
			case pm::Geometry::Type::RECTANGLE:
			{
				const pm::Rectangle *rectangle = static_cast<const pm::Rectangle *>(geometry);
				indent(file, amount).formatAppend("%s = {x = %f, y = %f, z = %f},\n", Names::point, rectangle->point().x, rectangle->point().y, rectangle->point().z);
				indent(file, amount).formatAppend("%s = {x = %f, y = %f, z = %f},\n", Names::sideA, rectangle->sideA().x, rectangle->sideA().y, rectangle->sideA().z);
				indent(file, amount).formatAppend("%s = {x = %f, y = %f, z = %f},\n", Names::sideB, rectangle->sideB().x, rectangle->sideB().y, rectangle->sideB().z);
				indent(file, amount).formatAppend("%s = {x = %f, y = %f, z = %f}\n", Names::normal, rectangle->normal().x, rectangle->normal().y, rectangle->normal().z);
				break;
			}
		}

		const bool isLastGeometry = (i == world.objects().size() - 1);
		amount--;
		indent(file, amount).formatAppend("}%s", isLastGeometry ? "" : ",\n");
		file.append("\n");
	}
	amount--;
	indent(file, amount).append("},\n");
	file.append("\n");

	// Lights serializer
	indent(file, amount).formatAppend("%s =\n", Names::lights);
	indent(file, amount).append("{\n");
	amount++;
	for (unsigned int i = 0; i < world.lights().size(); i++)
	{
		indent(file, amount).append("{\n");

		amount++;
		const pm::Light *light = world.lights()[i].get();
		const pm::Light::Type lightType = light->type();
		indent(file, amount).formatAppend("%s = \"%s\",\n", Names::type, lightTypeToString(lightType));
		indent(file, amount).formatAppend("%s = %s,\n", Names::castShadows, light->castShadows() ? "true" : "false");

		switch (lightType)
		{
			case pm::Light::Type::DIRECTIONAL:
			{
				const pm::Directional *directional = static_cast<const pm::Directional *>(light);
				indent(file, amount).formatAppend("%s = %f,\n", Names::radianceScale, directional->radianceScale());
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f},\n", Names::color, directional->color().r, directional->color().g, directional->color().b);
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f}\n", Names::direction, directional->direction().x, directional->direction().y, directional->direction().z);
				break;
			}
			case pm::Light::Type::POINT:
			{
				const pm::PointLight *pointLight = static_cast<const pm::PointLight *>(light);
				indent(file, amount).formatAppend("%s = %f,\n", Names::radianceScale, pointLight->radianceScale());
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f},\n", Names::color, pointLight->color().r, pointLight->color().g, pointLight->color().b);
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f}\n", Names::location, pointLight->location().x, pointLight->location().y, pointLight->location().z);
				break;
			}
			case pm::Light::Type::AMBIENT:
			{
				const pm::Ambient *ambient = static_cast<const pm::Ambient *>(light);
				indent(file, amount).formatAppend("%s = %f,\n", Names::radianceScale, ambient->radianceScale());
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f}\n", Names::color, ambient->color().r, ambient->color().g, ambient->color().b);
				break;
			}
			case pm::Light::Type::AMBIENT_OCCLUDER:
			{
				const pm::AmbientOccluder *ambientOccluder = static_cast<const pm::AmbientOccluder *>(light);
				indent(file, amount).formatAppend("%s = %f,\n", Names::radianceScale, ambientOccluder->radianceScale());
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f},\n", Names::color, ambientOccluder->color().r, ambientOccluder->color().g, ambientOccluder->color().b);
				indent(file, amount).formatAppend("%s = {r = %f, g = %f, b = %f},\n", Names::minAmount, ambientOccluder->minAmount().r, ambientOccluder->minAmount().g, ambientOccluder->minAmount().b);
				indent(file, amount).formatAppend("%s = %d,\n", Names::samplerIndex, samplerIndex(world, ambientOccluder->sampler()));
				break;
			}
			case pm::Light::Type::AREA:
			{
				const pm::AreaLight *areaLight = static_cast<const pm::AreaLight *>(light);
				indent(file, amount).formatAppend("%s = %d,\n", Names::objectIndex, geometryIndex(world, &areaLight->object()));
				break;
			}
			case pm::Light::Type::ENVIRONMENT:
			{
				const pm::EnvironmentLight *environmentLight = static_cast<const pm::EnvironmentLight *>(light);
				indent(file, amount).formatAppend("%s = %d,\n", Names::materialIndex, materialIndex(world, &environmentLight->material()));
				break;
			}
		}

		const bool isLastLight = (i == world.lights().size() - 1);
		amount--;
		indent(file, amount).formatAppend("}%s", isLastLight ? "" : ",\n");
		file.append("\n");
	}
	amount--;
	indent(file, amount).append("}\n");

	amount--;
	indent(file, amount).append("}\n");
	file.append("\n");

	nctl::UniquePtr<nc::IFile> fileHandle = nc::IFile::createFileHandle(filename);
	fileHandle->open(nc::IFile::OpenMode::WRITE | nc::IFile::OpenMode::BINARY);
	fileHandle->write(file.data(), file.length());
	fileHandle->close();
}
