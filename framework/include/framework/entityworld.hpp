#pragma once

#include <framework/entity.hpp>

#include <littl/List.hpp>

#if ZOMBIE_API_VERSION >= 201601
#include <functional>
#endif

namespace zfw
{
    class EntityWorld
    {
        public:
            EntityWorld(ISystem* sys) : sys(sys) {}
            
            bool AddEntity(shared_ptr<IEntity> ent);
            void AddEntityFilter(IEntityFilter* filter);
            void Draw(const UUID_t* modeOrNull);
            IEntity* GetEntityByID(int entID);
            size_t GetNumEntities() const { return entities.getLength(); }
			bool InitAllEntities();
            void OnFrame(double delta);
            void OnTick();
            void RemoveAllEntities(bool destroy);
            void RemoveEntity(IEntity* ent);
            void RemoveEntity(shared_ptr<IEntity> ent) { RemoveEntity(ent.get()); }
            void RemoveEntityFilter(IEntityFilter* filter);
			bool Serialize(OutputStream* output, int flags);
			bool Unserialize(InputStream* input, int flags);

            void WalkEntities(IEntityVisitor* visitor);

#if ZOMBIE_API_VERSION >= 201601
            // Identical in function to WalkEntities, but this name seems better
            // Iterates over every single entity in the world, think twice or thrice whether this is what you want
            void IterateEntities(std::function<void(IEntity* ent)> visit);
#endif

#if ZOMBIE_API_VERSION < 201601
            // When this is removed, also remove li::List dependency
            auto GetEntityList() -> li::List<shared_ptr<IEntity>>& { return entities; }
#endif

        protected:
            ISystem* sys;

            li::List<shared_ptr<IEntity>> entities;
            li::List<IEntityFilter*>    entityFilters;

        private:
            EntityWorld(const EntityWorld&) = delete;
    };

    class IEntityVisitor {
        public:
            virtual void Visit(IEntity* ent) = 0;
    };
}
