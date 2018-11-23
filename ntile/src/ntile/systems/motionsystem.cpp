#include "../components/aabbcollision.hpp"
#include "../components/motion.hpp"
#include "../systems/motionsystem.hpp"
#include "../world.hpp"

#include <framework/broadcasthandler.hpp>
#include <framework/components/position.hpp>
#include <framework/entityworld2.hpp>

namespace ntile
{
    using std::make_unique;

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class MotionSystem : public IMotionSystem {
    public:
        MotionSystem(IBroadcastHandler* broadcastHandler) : broadcastHandler(broadcastHandler) {}

        // ISystem
        void OnTicks(int ticks) final;

        // IMotionSystem
        void Attach(IEntityWorld2* world) final;

    private:
        IBroadcastHandler* broadcastHandler;
        IEntityWorld2* world = nullptr;
    };

    // ====================================================================== //
    //  class MotionSystem
    // ====================================================================== //

    unique_ptr<IMotionSystem> IMotionSystem::Create(IBroadcastHandler* broadcastHandler) {
        return make_unique<MotionSystem>(broadcastHandler);
    }

    void MotionSystem::Attach(IEntityWorld2* world) {
        this->world = world;
    }

    bool CheckCollisionAt(IEntityWorld2* world, intptr_t entityId, const AabbCollision& aabb, Float3& newPos)
    {
        Float3 min = newPos + aabb.min, max = newPos + aabb.max;

//        const Short2 blockXY = WorldToBlockXY(Float2(newPos));
//
//        for (int by = std::max(blockXY.y - 1, 0); by < blockXY.y + 1 && by < worldSize.y; by++)
//            for (int bx = std::max(blockXY.x - 1, 0); bx < blockXY.x + 1 && bx < worldSize.x; bx++)
//            {
//                Float3 ent_bbox[2];
//
//                iterate2 (i, blocks[by * worldSize.x + bx].entities)
//                {
//                    if (i == pe || !i->GetAABB(ent_bbox[0], ent_bbox[1]))
//                        continue;
//
//                    if (min.x < ent_bbox[1].x && ent_bbox[0].x < max.x
//                        && min.y < ent_bbox[1].y && ent_bbox[0].y < max.y
//                        && min.z < ent_bbox[1].z && ent_bbox[0].z < max.z)
//                        return false;
//                }
//            }

        static const float HEIGHT_THRESHOLD = 8.0f;

        const int minX = std::max<int>((int)floor(min.x / TILE_SIZE_H - 0.5f), 0);
        const int minY = std::max<int>((int)floor(min.y / TILE_SIZE_V - 0.5f), 0);
        const int maxX = std::min<int>((int)floor(max.x / TILE_SIZE_H - 0.5f), worldSize.x * TILES_IN_BLOCK_H);
        const int maxY = std::min<int>((int)floor(max.y / TILE_SIZE_V - 0.5f), worldSize.y * TILES_IN_BLOCK_V);

        float newZ = -1000.0f;

        for (int yy = minY; yy <= maxY; yy++)
        {
            const int by = yy / TILES_IN_BLOCK_V;

            if (by >= worldSize.y)
                break;

            WorldBlock* block = nullptr;
            WorldTile* p_tile = nullptr, * p_tile_east = nullptr, * p_tile_south = nullptr;

            for (int xx = minX; xx <= maxX; xx++)
            {
                if (p_tile == nullptr || xx % TILES_IN_BLOCK_H == 0)
                {
                    const int bx = xx / TILES_IN_BLOCK_H;

                    if (bx >= worldSize.x)
                        break;

                    block = &blocks[by * worldSize.x + bx];

                    p_tile = &block->tiles[yy % TILES_IN_BLOCK_V][xx % TILES_IN_BLOCK_H];
                    p_tile_east = &block->tiles[yy % TILES_IN_BLOCK_V][xx % TILES_IN_BLOCK_H + 1];

                    if (yy % TILES_IN_BLOCK_V == TILES_IN_BLOCK_V - 1)
                    {
                        WorldBlock* block_south = (by + 1 < worldSize.x) ? &blocks[(by + 1) * worldSize.x + bx] : nullptr;
                        p_tile_south = &block_south->tiles[0][xx % TILES_IN_BLOCK_H];
                    }
                    else
                        p_tile_south = &block->tiles[yy % TILES_IN_BLOCK_V + 1][xx % TILES_IN_BLOCK_H];
                }

                if (xx % TILES_IN_BLOCK_H == TILES_IN_BLOCK_H - 1)
                {
                    const int bx = xx / TILES_IN_BLOCK_H;

                    WorldBlock* block_east = (bx + 1 < worldSize.x) ? &blocks[by * worldSize.x + (bx + 1)] : nullptr;
                    p_tile_east = (block_east != nullptr) ? &block_east->tiles[yy % TILES_IN_BLOCK_V][0] : nullptr;
                }

                if (fabs(p_tile->elev - newPos.z) > HEIGHT_THRESHOLD)
                    return true;
                else
                    newZ = std::max(newZ, (float) p_tile->elev);

                p_tile++;

                // Wow! This might be a nullptr! (if we're on the edge of the map)
                // In that case though, it will be reset in the next iteration anyway
                p_tile_east++;

                if (p_tile_south != nullptr)
                    p_tile_south++;
            }
        }

        if (newZ > -999.0f)
            newPos.z = newZ;

        return false;
    }

    void MotionSystem::OnTicks(int ticks) {
        while (ticks--) {
            this->world->IterateEntitiesByComponent(Motion::GetType(), [this](intptr_t entityId, void *component_data) {
                auto motion = static_cast<Motion*>(component_data);
                auto position = this->world->GetEntityComponent<Position>(entityId);
                auto aabbCollision = this->world->GetEntityComponent<AabbCollision>(entityId);

                if (!position) {
                    return;
                }

                if (motion->ticksRemaining != 0) {
                    const Float3 newPos = position->pos + motion->speed;

                    position->pos = newPos;

                    motion->ticksRemaining--;
                }

                if (motion->ticksRemaining == 0) {
                    if (motion->nudgeDirection.x != 0 || motion->nudgeDirection.y != 0) {
                        // Just to be sure
                        motion->nudgeDirection.x = std::min(1, std::max(-1, motion->nudgeDirection.x));
                        motion->nudgeDirection.y = std::min(1, std::max(-1, motion->nudgeDirection.y));

                        // Update player angle
                        auto angle = (float) atan2(-motion->nudgeDirection.y, motion->nudgeDirection.x);
                        position->rotation = glm::fquat(Float3(0.0f, 0.0f, -angle));

                        auto pos = position->pos;
                        auto newX = pos.x + TILE_SIZE_H * (float) motion->nudgeDirection.x;
                        auto newY = pos.y + TILE_SIZE_V * (float) motion->nudgeDirection.y;

                        Float3 newPos = Float3(newX, newY, pos.z);

                        if (aabbCollision != nullptr)
                        {
                            if (CheckCollisionAt(world, entityId, *aabbCollision, newPos))
                            {
                                newPos = Float3(pos.x, newY, pos.z);

                                if (CheckCollisionAt(world, entityId, *aabbCollision, newPos))
                                {
                                    newPos = Float3(newX, pos.y, pos.z);

                                    if (CheckCollisionAt(world, entityId, *aabbCollision, newPos))
                                        return;
                                }
                            }
                        }

                        // Move for 16 ticks
                        motion->speed = (newPos - pos) * (1.0f / 16);
                        motion->ticksRemaining = 16;

                        AnimationTrigerEvent trigger;
                        trigger.entityId = entityId;
                        trigger.animationName = (motion->lastAnim == 0) ? "step" : "step2";
                        motion->lastAnim = 1 - motion->lastAnim;
                        broadcastHandler->BroadcastMessage(trigger);
                    }
                }
            });
        }
    }
}
