#include "s_replication.h"
#include "sol/sol.h"
#include "sol_core.h"
#include "world.h"
#include "network.h"
#include "event/s_event.h"

#define CORRECT_NONE 0.1f
#define CORRECT_LIGHT 1.0f
#define CORRECT_MED 4.0f

static void Sync_Buffs(World *world, int id, u32 serverMask);

u32 event_kinds_replicate[EVENTKIND_COUNT] = {
    [EVENTKIND_FX]      = 1,
    [EVENTKIND_HIT]     = 1,
    [EVENTKIND_RESPAWN] = 1,
    [EVENTKIND_DEATH]   = 1,
};

void Sol_Replication_Init(World *world)
{
    world->replications = calloc(MAX_ENTS, sizeof(CompReplication));
    world->worldNet     = calloc(1, sizeof(WorldNet));
}

void Sol_Replication_Add(World *world, int id, NetAuth auth, u8 prefabKind)
{
    CompReplication rep = {
        .auth       = auth,
        .prefabKind = prefabKind,
    };
    world->masks[id] |= BITC(HAS_REPLICATION);
    world->replications[id] = rep;

    if (auth == NETAUTH_LOCAL)
    {
        if (world->worldNet->predictionCount <= MAX_NET_PREDICTIONS)
            return;
        u32         indx = (world->worldNet->predictionCount + 1) % MAX_NET_PREDICTIONS;
        Prediction *p    = &world->worldNet->predictions[indx];
        p->localEntId    = id;
        p->prefabKind    = prefabKind;
        p->tickSpawned   = world->currentTick;
    }
}

void Sol_Replication_Disconnect(World *world)
{
    if (!world || !world->worldNet)
        return;

    WorldNet *wNet = world->worldNet;

    // 1. Loop through and clear out mapping tracking
    for (int hostId = 0; hostId <= wNet->maxHostId; hostId++)
    {
        int localId = wNet->hostToLocalMap[hostId];
        if (localId > 1)
        {
            // Destroy the entities remaining from the previous session
            Sol_Destroy_Ent(world, localId);
        }
    }

    // 2. Clear out tracking arrays completely
    memset(wNet->hostToLocalMap, 0, sizeof(wNet->hostToLocalMap));
    memset(wNet->snapShots, 0, sizeof(wNet->snapShots));
    memset(wNet->seenThisSnap, 0, sizeof(wNet->seenThisSnap));

    wNet->snapHead  = 0;
    wNet->maxHostId = 0;
}

void Net_Send_Snap(World *world)
{
    static WorldSnap snap;
    memset(&snap, 0, sizeof(WorldSnap));
    snap.type       = NET_PACKET_SNAPSHOT;
    snap.worldId    = world->worldId;
    snap.tickNumber = world->currentTick;

    int count = 0;
    for (int i = 0; i < world->activeCount && count < MAX_NET_ENTS; i++)
    {
        int id = world->activeEntities[i];
        if (world->replications[id].auth != NETAUTH_AUTH)
            continue;

        NetEntityState *e = &snap.entities[count++];
        e->id             = id;
        e->compMask       = world->masks[id];
        e->prefabKind     = world->replications[id].prefabKind;
        e->pos            = world->xforms[id].pos;
        e->rot            = world->xforms[id].quat;
        e->scale          = world->xforms[id].scale.x;
        if (world->masks[id] & BITC(HAS_CONTROLLER))
        {
            e->yaw   = world->controllers[id].yaw;
            e->pitch = world->controllers[id].pitch;
        }
        if (world->masks[id] & BITC(HAS_MODEL))
        {
            e->modelId        = world->models[id].modelId;
            e->leftWeaponEnt  = world->models[id].leftWeaponEnt;
            e->rightWeaponEnt = world->models[id].rightWeaponEnt;
            // e->weapons[0].entId = world->models[id].leftWeaponEnt;
            // e->weapons[0].modelId = Sol_Model_GetModelId(world, world->models[id].leftWeaponEnt);

            // e->weapons[1].entId = world->models[id].rightWeaponEnt;
            // e->weapons[1].modelId = Sol_Model_GetModelId(world, world->models[id].rightWeaponEnt);

            for (int layer = 0; layer < ANIM_LAYER_COUNT; layer++)
            {
                if (world->models[id].layers[layer].fadeOut > 0)
                    continue;
                e->animCurrent[layer]  = world->models[id].layers[layer].animId;
                e->animSeek[layer]     = world->models[id].layers[layer].currentSeek;
                e->animSpeed[layer]    = world->models[id].layers[layer].playRate;
                e->animPlayKind[layer] = world->models[id].layers[layer].playKind;
            }
        }
        if (world->masks[id] & BITC(HAS_ABILITY))
        {
            CompAbility *a    = &world->abilities[id];
            AbilityData *data = &world->abilities[id].stateData[a->activeSlot];

            e->abilityState  = a->state;
            e->activeSlot    = a->activeSlot;
            e->abilityCharge = data->charge;
            e->abilityStage  = data->stage;
            for (int slot = 0; slot < MAX_MAPPED_SKILLS; slot++)
            {
                e->bindingState[slot]        = world->abilities[id].bindings[slot].boundState;
                e->bindingRarity[slot]       = world->abilities[id].bindings[slot].boundRarity;
                e->bindingBonusdamage[slot]  = world->abilities[id].bindings[slot].boundBonusDamage;
                e->bindingBonusBuffs[slot]   = world->abilities[id].bindings[slot].boundBonusBuffs;
                e->bindingBonusEffects[slot] = world->abilities[id].bindings[slot].boundBonusEffects;
            }
        }

        if (world->masks[id] & BITC(HAS_BODY3))
        {
            e->vel    = world->bodies[id].vel;
            e->height = world->bodies[id].dims.y;
        }
        if (world->masks[id] & BITC(HAS_OWNER))
        {
            e->ownerId = world->owners[id].ownerId;
            e->team    = world->owners[id].team;
        }
        if (world->masks[id] & BITC(HAS_VITAL))
        {
            e->health = world->vitals[id].health;
            e->energy = world->vitals[id].energy;
        }
        if (world->masks[id] & BITC(HAS_BUFF))
        {
            e->buffMask = Sol_Buff_GetMask(world, id);
        }
    }
    snap.eCount = count; // ← outside the loop

    size_t sendSize = offsetof(WorldSnap, entities) + sizeof(NetEntityState) * count;

    ENetPacket *packet = enet_packet_create(&snap, sendSize, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_host_broadcast(solNet.host, 0, packet);
}

void Net_Apply_Snap(World *world)
{
    WorldNet *net = world->worldNet;

    // Check an explicit dynamic sequence number or an initialization flag instead of index positions
    u32        idx  = (net->snapHead + MAX_SNAPS_BUFFERED - 1) % MAX_SNAPS_BUFFERED;
    WorldSnap *snap = &net->snapShots[idx];

    // If we haven't processed any ticks yet, there's nothing to do
    if (snap->tickNumber == 0 && snap->eCount == 0)
        return;

    memset(net->seenThisSnap, 0, sizeof(net->seenThisSnap));

    for (int i = 0; i < snap->eCount; i++)
    {
        NetEntityState *e      = &snap->entities[i];
        int             hostId = e->id;
        if (hostId >= MAX_ENTS)
            continue;

        net->seenThisSnap[hostId] = true;
        int id                    = net->hostToLocalMap[hostId];

        if (id == 0)
        {
            if (e->prefabKind)
                id = Sol_Prefab_Factory(world, 0, e->prefabKind,
                                        (EntDesc){
                                            .pos       = e->pos,
                                            .rot       = e->rot,
                                            .scale     = e->scale,
                                            .authority = NETAUTH_REMOTE,
                                        });
            else
            {
                id = Sol_Create_Ent(world, 0);
                Sol_Replication_Add(world, id, NETAUTH_REMOTE, 0);
            }

            net->hostToLocalMap[hostId] = id;
            if (hostId > net->maxHostId)
                net->maxHostId = hostId;
        }
        world->masks[id]        = e->compMask;
        world->xforms[id].scale = glms_vec3_fill(e->scale);

        float dist = glms_vec3_distance2(world->xforms[id].pos, e->pos);
        if (world->replications[id].auth == NETAUTH_AUTH)
        {
            if (dist > CORRECT_NONE)
            {
                if (dist < CORRECT_LIGHT)
                    world->xforms[id].pos = glms_vec3_lerp(world->xforms[id].pos, e->pos, (float)SOL_TIMESTEP);
                else
                {
                    float t                = 1.0f - expf(-dist / CORRECT_MED);
                    world->xforms[id].pos  = glms_vec3_lerp(world->xforms[id].pos, e->pos, t);
                    world->bodies[id].vel  = e->vel;
                    world->xforms[id].quat = e->rot;
                }
            }
        }
        else if (world->replications[id].auth == NETAUTH_REMOTE)
        {
            if (dist < MAX_NET_INTERP_DISTANCE)
                world->xforms[id].pos = glms_vec3_lerp(world->xforms[id].pos, e->pos, (float)SOL_TIMESTEP);
            else
                world->xforms[id].pos = e->pos;

            world->xforms[id].quat = e->rot;
            if (world->masks[id] & BITC(HAS_CONTROLLER))
            {
                world->controllers[id].yaw   = e->yaw;
                world->controllers[id].pitch = e->pitch;
            }
            if (world->masks[id] & BITC(HAS_BODY3))
            {
                world->bodies[id].vel    = e->vel;
                world->bodies[id].dims.y = e->height;
            }

            if (world->masks[id] & BITC(HAS_MODEL))
            {
                world->models[id].modelId        = e->modelId;
                sollog(world->models[id].leftWeaponEnt);
                world->models[id].leftWeaponEnt  = world->worldNet->hostToLocalMap[e->leftWeaponEnt];
                world->models[id].rightWeaponEnt = world->worldNet->hostToLocalMap[e->rightWeaponEnt];

                for (int layer = 0; layer < ANIM_LAYER_COUNT; layer++)
                {
                    Sol_Model_PlayAnim(world, id,
                                       (AnimDesc){
                                           .layerId  = layer,
                                           .anim     = e->animCurrent[layer],
                                           .seek     = e->animSeek[layer],
                                           .speed    = e->animSpeed[layer],
                                           .playKind = e->animPlayKind[layer],
                                       });
                    Sol_Model_SetAnimSpeed(world, id, layer, e->animSpeed[layer]);
                }
            }
            if (world->masks[id] & BITC(HAS_ABILITY))
            {
                CompAbility *ability = &world->abilities[id];
                for (int slot = 0; slot < MAX_MAPPED_SKILLS; slot++)
                {
                    SkillBinding *b      = &ability->bindings[slot];
                    b->boundState        = e->bindingState[slot];
                    b->boundRarity       = e->bindingRarity[slot];
                    b->boundBonusDamage  = e->bindingBonusdamage[slot];
                    b->boundBonusBuffs   = e->bindingBonusBuffs[slot];
                    b->boundBonusEffects = e->bindingBonusEffects[slot];
                    // pendingState/dirty stays as-is — if the client is still trying
                    // to bind something different, the next Send_Input will send it again
                }
                ability->state      = e->abilityState;
                ability->activeSlot = e->activeSlot;
                AbilityData *data   = &ability->stateData[ability->activeSlot];
                data->charge        = e->abilityCharge;
                data->stage         = e->abilityStage;
            }
        }
        Sync_Buffs(world, id, e->buffMask);

        if (world->masks[id] & BITC(HAS_OWNER))
        {
            world->owners[id].ownerId = net->hostToLocalMap[e->ownerId];
            world->owners[id].team    = e->team;
        }

        if (world->masks[id] & BITC(HAS_VITAL))
        {
            world->vitals[id].health = e->health;
            world->vitals[id].energy = e->energy;
        }
    }
    // Find entities that were mapped but aren't in this snap → destroy
    for (int hostId = 0; hostId <= net->maxHostId; hostId++)
    {
        int localId = net->hostToLocalMap[hostId];
        if (localId != 0 && !net->seenThisSnap[hostId])
        {
            Sol_Destroy_Ent(world, localId);
            net->hostToLocalMap[hostId] = 0;
        }
    }
}

void Net_Send_Input(World *world)
{
    if (!(world->masks[1] & BITC(HAS_CONTROLLER)))
        return;
    CompController *controller = &world->controllers[1];

    NetInputPacket inputPacket = {
        .type        = NET_PACKET_INPUT,
        .actionMask  = controller->actionState,
        .lookdir     = controller->lookdir,
        .wishdir     = controller->wishdir,
        .aimdir      = controller->aimdir,
        .yaw         = controller->yaw,
        .pitch       = controller->pitch,
        .isStrafing  = controller->isStrafing,
        .currentTick = world->currentTick,
    };
    CompAbility *a = &world->abilities[1];
    for (int i = 0; i < MAX_MAPPED_SKILLS; i++)
    {
        if (a->bindings[i].dirtySend)
        {
            inputPacket.hasEquipRequest = true;
            break;
        }
    }

    if (inputPacket.hasEquipRequest)
    {
        for (int i = 0; i < MAX_MAPPED_SKILLS; i++)
        {
            inputPacket.abilities[i] = a->bindings[i].pendingState;
            inputPacket.rarity[i]    = a->bindings[i].pendingRarity;
            inputPacket.addDamage[i] = a->bindings[i].pendingBonusDamage;
            inputPacket.addBuff[i]   = a->bindings[i].pendingBonusBuffs;
            inputPacket.addEffect[i] = a->bindings[i].pendingBonusEffects;
            a->bindings[i].dirtySend = false; // ack: we've sent this request
        }
    }
    ENetPacket *packet = enet_packet_create(&inputPacket, sizeof(NetInputPacket), ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    enet_peer_send(solNet.peer, 0, packet);
}

void Net_Send_Events(World *world)
{
    EventSnap snap  = {0};
    snap.type       = NET_PACKET_EVENT;
    snap.worldId    = world->worldId;
    snap.tickNumber = world->currentTick;

    u32 count = 0;
    for (int i = 0; i < world->events->count && count < MAX_NET_EVENTS; i++)
    {
        SolEvent *e = &world->events->event[i];
        if (event_kinds_replicate[e->kind])
        {
            snap.events[count++] = *e;
        }
    }
    if (count == 0)
        return;
    snap.eventCount = count;

    size_t      sendSize = offsetof(EventSnap, events) + sizeof(SolEvent) * count;
    ENetPacket *packet   = enet_packet_create(&snap, sendSize, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(solNet.host, 1, packet);
}

void Net_Apply_Events(World *world, EventSnap *snap)
{
    for (int i = 0; i < snap->eventCount; i++)
    {
        SolEvent e = snap->events[i];
        switch (e.kind)
        {
        case EVENTKIND_FX:
            e.as.fx.entA = world->worldNet->hostToLocalMap[e.as.fx.entA];
            e.as.fx.entB = world->worldNet->hostToLocalMap[e.as.fx.entB];
            break;
        case EVENTKIND_HIT:
            e.as.hit.entA = world->worldNet->hostToLocalMap[e.as.hit.entA];
            e.as.hit.entB = world->worldNet->hostToLocalMap[e.as.hit.entB];
            break;
        case EVENTKIND_DEATH:
            e.as.death.entA = world->worldNet->hostToLocalMap[e.as.death.entA];
            e.as.death.entB = world->worldNet->hostToLocalMap[e.as.death.entB];
            break;
        case EVENTKIND_RESPAWN:
            e.as.respawn.ent = world->worldNet->hostToLocalMap[e.as.respawn.ent];
            break;
        default:
            printf("Unhandled EventKind %d\n", e.kind);
            continue;
        }
        Sol_Event_Add(world, e);
    }
}

static void Sync_Buffs(World *world, int id, u32 serverMask)
{
    u32 clientMask = Sol_Buff_GetMask(world, id);

    if (clientMask != serverMask)
    {
        // XOR isolates the bits that do not match
        u32 changedBits = clientMask ^ serverMask;

        for (int b = 0; b < BUFFKIND_COUNT; b++)
        {
            u32 bit = (1 << b);
            if (changedBits & bit)
            {
                if (serverMask & bit)
                {
                    // Server has it, client doesn't -> Force Add (triggers onApply)
                    Sol_Buff_Add(world, id, 0, (BuffKind)b);
                }
                else
                {
                    // Client has it, server doesn't -> Force Remove (triggers onRemove)
                    Sol_Buff_Remove(world, id, (BuffKind)b);
                }
            }
        }
    }
}