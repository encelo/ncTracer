#ifndef CLASS_LUASERIALIZER
#define CLASS_LUASERIALIZER

namespace pm {
class World;
}

/// Lua world loader and saver
class LuaSerializer
{
  public:
	static bool load(const char *filename, pm::World &world);
	static void save(const char *filename, const pm::World &world);
};

#endif
