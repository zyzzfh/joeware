#pragma once

#include <functional>

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"

#include "AnimState.h"
#include "ClientClass.h"
#include "CommandContext.h"
#include "Cvar.h"
#include "CStudioHdr.h"
#include "Datamap.h"
#include "Engine.h"
#include "EngineTrace.h"
#include "EntityList.h"
#include "GlobalVars.h"
#include "LocalPlayer.h"
#include "matrix3x4.h"
#include "MDLCache.h"
#include "ModelInfo.h"
#include "ModelRender.h"
#include "Utils.h"
#include "UtlVector.h"
#include "VarMapping.h"
#include "Vector.h"
#include "VirtualMethod.h"
#include "WeaponData.h"
#include "WeaponId.h"

struct AnimState;

struct AnimationLayer
{
public:
    float animationTime; //0
    float fadeOut; //4
    CStudioHdr* dispatchedStudioHdr; //8
    int dispatchedSrc; //12
    int dispatchedDst; //16
    unsigned int order; //20, networked
    unsigned int sequence; //24, networked
    float prevCycle; //28, networked
    float weight; //32, networked
    float weightDeltaRate; //36, networked
    float playbackRate; //40, networked
    float cycle; //44, networked
    void* owner; //48
    int invalidatePhysicsBits; //52

    void reset()
    {
        sequence = 0;
        weight = 0;
        weightDeltaRate = 0;
        playbackRate = 0;
        prevCycle = 0;
        cycle = 0;
    }
};

enum PlayerFlag
{
    PlayerFlag_OnGround = 1 << 0, // At rest / on the ground
    PlayerFlag_Crouched = 1 << 1, // Player is fully crouched
    PlayerFlag_WaterJump = 1 << 2, // Player jumping out of water
    PlayerFlag_OnTrain = 1 << 3, // Player is controlling a train, so movement commands should be ignored on client during prediction
    PlayerFlag_InRain = 1 << 4, // Indicates the entity is standing in rain
    PlayerFlag_Frozen = 1 << 5, // Player is frozen for 3rd person camera
    PlayerFlag_AtControls = 1 << 6, // Player can't move, but keeps key inputs for controlling another entity
    PlayerFlag_Client = 1 << 7, // Is a player
    PlayerFlag_FakeClient = 1 << 8, // Fake client, simulated server side; don't send network messages to them. NON-PLAYER SPECIFIC (i.e. not used by GameMovement or the client.dll) - can still be applied to players, though
    PlayerFlag_InWater = 1 << 9, // In water
    PlayerFlag_Fly = 1 << 10, // Changes the SV_Movestep() behavior to not need to be on ground
    PlayerFlag_Swim = 1 << 11, // Changes the SV_Movestep() behavior to not need to be on ground (but stay in water)
    PlayerFlag_Conveyor = 1 << 12,
    PlayerFlag_NPC = 1 << 13,
    PlayerFlag_GodMode = 1 << 14,
    PlayerFlag_NoTarget = 1 << 15,
    PlayerFlag_AimTarget = 1 << 16, // Set if the crosshair needs to aim onto the entity
    PlayerFlag_PartialGround = 1 << 17, // Not all corners are valid
    PlayerFlag_StaticProp = 1 << 18, // Eetsa static prop!
    PlayerFlag_Graphed = 1 << 19, // Worldgraph has this ent listed as something that blocks a connection
    PlayerFlag_Grenade = 1 << 20,
    PlayerFlag_StepMovement = 1 << 21, // Changes the SV_Movestep() behavior to not do any processing
    PlayerFlag_NoTouch = 1 << 22, // Doesn't generate touch functions, generates Untouch() for anything it was touching when this flag was set
    PlayerFlag_BaseVelocity = 1 << 23, // Base velocity has been applied this frame (used to convert base velocity into momentum)
    PlayerFlag_WorldBrush = 1 << 24, // Not moveable/removeable brush entity (really part of the world, but represented as an entity for transparency or something)
    PlayerFlag_NPCSpecific = 1 << 25, // This is an object that NPCs should see. Missiles, for example
    PlayerFlag_KillMe = 1 << 26, // This entity is marked for death -- will be freed by game DLL
    PlayerFlag_OnFire = 1 << 27, // You know...
    PlayerFlag_Dissolving = 1 << 28, // We're dissolving!
    PlayerFlag_TransitionRagdoll = 1 << 29, // In the process of turning into a client side ragdoll
    PlayerFlag_NotBlockableByPlayer = 1 << 30 // Pusher that can't be blocked by the player
};

enum EffectFlag
{
    EffectFlag_BoneMerge = 1 << 0, // Performs bone merge on client side
    EffectFlag_BrightLight = 1 << 1, // Dlight centered at entity origin
    EffectFlag_Flashlight = 1 << 2, // Player flashlight
    EffectFlag_NoInterp = 1 << 3, // Don't interpolate the next frame
    EffectFlag_NoShadow = 1 << 4, // Don't cast shadow
    EffectFlag_NoDraw = 1 << 5, // Don't draw entity
    EffectFlag_NoRecieveShadow = 1 << 6, // Don't receive shadow
    EffectFlag_BoneMergeFastCull = 1 << 7, // For use with EffectFlag_BoneMerge
    EffectFlag_ItemBlink = 1 << 8, // Blink an item so that the player notices it
    EffectFlag_ParentAnimates = 1 << 9, // Always assume that the parent entity is animating
    EffectFlag_FastReflection = 1 << 10, // Marks an entity for reflection rendering when using $reflectonlymarkedentities material variable
    EffectFlag_NoShadowDepth = 1 << 11, // Indicates this entity does not render into any shadow depthmap
    EffectFlag_NoShadowDepthCache = 1 << 12, // Indicates this entity cannot be cached in shadow depthmap and should render every frame
    EffectFlag_NoFlashlight = 1 << 13,
    EffectFlag_NoCSM = 1 << 14 // Indicates this entity does not render into the cascade shadow depthmap
};

enum class MoveType {
    NOCLIP = 8,
    LADDER = 9
};

enum class ObsMode {
    None = 0,
    Deathcam,
    Freezecam,
    Fixed,
    InEye,
    Chase,
    Roaming
};

enum class Team {
    None = 0,
    Spectators,
    TT,
    CT
};

class Collideable {
public:
    PAD(8); //0
    Vector vecMins; //8(x), 12(y), 16(z) 
    Vector vecMaxs; //20(x), 24(y), 28(z)
    unsigned short solidFlags; //32
    unsigned char solidType; //34
    unsigned char triggerBloat; //35
    float radius; //36

    VIRTUAL_METHOD(Vector&, obbMins, 1, (), (this))
        VIRTUAL_METHOD(Vector&, obbMaxs, 2, (), (this))

        void setCollisionBounds(const Vector& mins, const Vector& maxs)
    {
        if ((vecMins == mins) && (vecMaxs == maxs))
            return;

        vecMins = mins;
        vecMaxs = maxs;

        Vector vecSize;
        Vector::vectorSubtract(maxs, mins, vecSize);
        radius = vecSize.length() * 0.5f;

        memory->markSurroundingBoundsDirty(this);
    }
};


class Entity {
public:

    VIRTUAL_METHOD(void, release, 1, (), (this + sizeof(uintptr_t) * 2))
        VIRTUAL_METHOD(ClientClass*, getClientClass, 2, (), (this + sizeof(uintptr_t) * 2))
        VIRTUAL_METHOD(void, preDataUpdate, 6, (int updateType), (this + sizeof(uintptr_t) * 2, updateType))
        VIRTUAL_METHOD(void, postDataUpdate, 7, (int updateType), (this + sizeof(uintptr_t) * 2, updateType))
        VIRTUAL_METHOD(bool, isDormant, 9, (), (this + sizeof(uintptr_t) * 2))
        VIRTUAL_METHOD(int, index, 10, (), (this + sizeof(uintptr_t) * 2))
        VIRTUAL_METHOD(void, setDestroyedOnRecreateEntities, 13, (), (this + sizeof(uintptr_t) * 2))

        VIRTUAL_METHOD(Vector&, getRenderOrigin, 1, (), (this + sizeof(uintptr_t)))
        VIRTUAL_METHOD(Vector&, getRenderAngles, 2, (), (this + sizeof(uintptr_t)))
        VIRTUAL_METHOD(bool, shouldDraw, 3, (), (this + sizeof(uintptr_t)))
        VIRTUAL_METHOD(const Model*, getModel, 8, (), (this + sizeof(uintptr_t)))
        VIRTUAL_METHOD(const matrix3x4&, toWorldTransform, 32, (), (this + sizeof(uintptr_t)))

        VIRTUAL_METHOD_V(int&, handle, 2, (), (this))
        VIRTUAL_METHOD_V(Collideable*, getCollideable, 3, (), (this))

        VIRTUAL_METHOD(const Vector&, getAbsOrigin, 10, (), (this))
        VIRTUAL_METHOD(Vector&, getAbsAngle, 11, (), (this))
        VIRTUAL_METHOD(datamap*, getDataDescMap, 15, (), (this))
        VIRTUAL_METHOD(datamap*, getPredDescMap, 17, (), (this))
        VIRTUAL_METHOD(void, setModelIndex, 75, (int index), (this, index))
        VIRTUAL_METHOD(bool, getAttachment, 84, (int index, Vector& origin), (this, index, std::ref(origin)))
        VIRTUAL_METHOD(Team, getTeamNumber, 88, (), (this))
        VIRTUAL_METHOD(int, health, 122, (), (this))
        VIRTUAL_METHOD(int, maxHealth, 123, (), (this))
        VIRTUAL_METHOD(void, think, 139, (), (this))
        VIRTUAL_METHOD(bool, isAlive, 156, (), (this))
        VIRTUAL_METHOD(bool, isPlayer, 158, (), (this))
        VIRTUAL_METHOD(bool, isWeapon, 166, (), (this))
        VIRTUAL_METHOD(void, setSequence, 219, (int sequence), (this, sequence))
        VIRTUAL_METHOD(void, studioFrameAdvance, 220, (), (this))
        VIRTUAL_METHOD(float, getLayerSequenceCycleRate, 223, (AnimationLayer* layer, int sequence), (this, layer, sequence))
        VIRTUAL_METHOD(void, updateClientSideAnimation, 224, (), (this))
        VIRTUAL_METHOD(Entity*, getActiveWeapon, 268, (), (this))
        VIRTUAL_METHOD(int, getWeaponSubType, 282, (), (this))
        VIRTUAL_METHOD(ObsMode, getObserverMode, 294, (), (this))
        VIRTUAL_METHOD(Entity*, getObserverTarget, 295, (), (this))
        VIRTUAL_METHOD(void, preThink, 318, (), (this))
        VIRTUAL_METHOD(void, updateCollisionBounds, 340, (), (this))
        VIRTUAL_METHOD(float, getMaxSpeed, 442, (), (this))
        VIRTUAL_METHOD(WeaponType, getWeaponType, 455, (), (this))
        VIRTUAL_METHOD(WeaponInfo*, getWeaponData, 461, (), (this))
        VIRTUAL_METHOD(int, getMuzzleAttachmentIndex1stPerson, 468, (Entity* viewModel), (this, viewModel))
        VIRTUAL_METHOD(int, getMuzzleAttachmentIndex3rdPerson, 469, (), (this))
        VIRTUAL_METHOD(float, getInaccuracy, 483, (), (this))
        VIRTUAL_METHOD(float, getSpread, 453, (), (this))
        VIRTUAL_METHOD(void, updateAccuracyPenalty, 484, (), (this))


        float sequenceDuration(int sequence) noexcept
    {
        float returnValue;
        memory->sequenceDuration(this, sequence);
        __asm movss returnValue, xmm0;
        return returnValue;
    }
    bool isVip() noexcept;
    bool isBot()
    {
        if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(index(), playerInfo))
            return playerInfo.fakeplayer;
        return false;
    }
    float getSequenceCycleRate(int sequence) noexcept
    {
        const float t = sequenceDuration(sequence);
        if (t > 0.0f)
            return 1.0f / t;
        else
            return 1.0f / 0.1f;
    }

    float GetDSYDelta() noexcept
    {
        auto animstate = this->getAnimstate();
        auto speedfraction = std::max<float>(0.f, std::min<float>(animstate->speedAsPortionOfWalkTopSpeed, 1.f));
        auto speedfactor = std::max<float>(0.f, std::min<float>(animstate->speedAsPortionOfCrouchTopSpeed, 1.f));

        auto lol = ((animstate->walkToRunTransition * -0.30000001f) - 0.19999999f) * speedfraction + 1.f;

        if (animstate->animDuckAmount > 0.0f)
            lol += ((animstate->animDuckAmount * speedfactor) * (0.5f - lol));

        return (animstate->aimYawMax * lol);
    }

    auto getEyePosition() noexcept
    {
        Vector v;
        VirtualMethod::call<void, 169>(this, std::ref(v));
        return v;
    }

    auto getAimPunch() noexcept
    {
        Vector v;
        VirtualMethod::call<void, 346>(this, std::ref(v));
        return v;
    }

    auto drawServerHitboxes(float duration = 0.f, int monocolor = 0)
    {
        static auto address = memory->drawServerHitboxes;

        auto player = memory->utilPlayerByIndex(index());
        if (!player)
            return;

        __asm {

            pushad

            movss xmm1, duration
            push monocolor
            mov ecx, player
            call address

            popad
        }

    }

    float getFirstSequenceAnimTag(int sequence, int animTag) noexcept
    {
        float returnValue;
        memory->getFirstSequenceAnimTag(this, sequence, animTag, 0);
        __asm movss returnValue, xmm0
        return returnValue;
    }

    float setPoseParameter(float value, int index) noexcept
    {
        static auto address = memory->studioSetPoseParameter;

        CStudioHdr* studioHdr = getModelPtr();
        if (!studioHdr)
            return value;

        if (index >= 0)
        {
            float newValue;

            __asm {

                pushad

                movss xmm2, [value]
                lea ecx, [newValue]
                push ecx
                mov edx, index
                mov ecx, studioHdr
                call address
                pop ecx

                popad
            }
            getPoseParameter()[index] = newValue;
        }
        return value;
    }

    int lookupSequence(const char* sequence) noexcept
    {
        return memory->lookUpSequence(this, sequence);
    }

    void getSequenceLinearMotion(CStudioHdr* studioHdr, int sequence, Vector& v) noexcept;

    float getSequenceMoveDist(CStudioHdr* studioHdr, int sequence) noexcept;

    CStudioHdr* getModelPtr() noexcept
    {
        return *reinterpret_cast<CStudioHdr**>(reinterpret_cast<uintptr_t>(this) + 0x2950);
    }

    Vector& getAbsVelocity() noexcept
    {
        static unsigned int m_vecAbsVelocity = DataMap::findInDataMap(getPredDescMap(), "m_vecAbsVelocity");
        return *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(this) + m_vecAbsVelocity);
    }

    Vector& callGetAbsVelocity() noexcept
    {
        static unsigned int m_vecAbsVelocity = DataMap::findInDataMap(getPredDescMap(), "m_vecAbsVelocity");
        memory->calcAbsoluteVelocity(this);
        return *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(this) + m_vecAbsVelocity);
    }

    Entity* groundEntity() noexcept
    {
        static unsigned int m_hGroundEntity = DataMap::findInDataMap(getPredDescMap(), "m_hGroundEntity");
        const auto handle = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + m_hGroundEntity);
        if (handle == -1)
            return nullptr;
        return interfaces->entityList->getEntityFromHandle(handle);
    }

    int getAnimationLayersCount()
    {
        return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x299C);
    }

    AnimationLayer* animOverlays() noexcept
    {
        //return *reinterpret_cast<AnimationLayer**>(reinterpret_cast<uintptr_t>(memory->animOverlay) + 0x1DA330 );
        return *reinterpret_cast<AnimationLayer**>(reinterpret_cast<uintptr_t>(this) + 0x2990);
    }

    AnimationLayer* getAnimationLayer(int overlay) noexcept
    {
        return &animOverlays()[overlay];
    }

    float* getPoseParameter() noexcept
    {
        static auto m_flPoseParameter = Netvars::get(fnv::hash("CBaseAnimating->m_flPoseParameter"));
        return reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + m_flPoseParameter);
    }

    std::array<float, 24>& poseParameters() noexcept
    {
        static auto m_flPoseParameter = Netvars::get(fnv::hash("CBaseAnimating->m_flPoseParameter"));
        return *reinterpret_cast<std::add_pointer_t<std::array<float, 24>>>(reinterpret_cast<uintptr_t>(this) + m_flPoseParameter);
    }

    void createState(AnimState* state) noexcept
    {
        static auto createAnimState = reinterpret_cast<void(__thiscall*)(AnimState*, Entity*)>(memory->createState);
        if (!this || !createAnimState)
            return;

        createAnimState(state, this);
    }

    void updateState(AnimState* state, Vector angle) noexcept
    {
        if (!this || !state || angle.null())
            return;

        static auto updateAnimState = reinterpret_cast<void(__vectorcall*)(void*, void*, float, float, float, void*)>(memory->updateState);
        if (!updateAnimState)
            return;

        updateAnimState(state, nullptr, 0.0f, angle.y, angle.x, nullptr);

    }

    void resetState(AnimState* state) noexcept
    {
        if (!state)
            return;

        static auto resetAnimState = reinterpret_cast<void(__thiscall*)(AnimState*)>(memory->resetState);
        if (!resetAnimState)
            return;

        resetAnimState(state);
    }

    const size_t getIntermidateDataSize() noexcept
    {
        const auto datamap = getPredDescMap();
        /*
        TODO: complete this
        if (datamap && !datamap->optimizedDataMap)
            BuildFlattenedChains(datamap);
        */
        size_t intermediateDataSize = 4;

        if (datamap->packedSize > 4)
            intermediateDataSize = datamap->packedSize;
        return intermediateDataSize;
    }

    void* getOriginalNetworkDataObject() noexcept
    {
        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(this) + 0x92C);
    }

    void* getPredictedFrame(int frameNumber) noexcept
    {
        if (!getOriginalNetworkDataObject())
            return nullptr;
        return reinterpret_cast<void**>(this)[(frameNumber % 150) + 286];
    }

    float spawnTime() noexcept
    {
        return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x103C0);
    }

    CommandContext* getCommandContext() noexcept
    {
        return reinterpret_cast<CommandContext*>(reinterpret_cast<uintptr_t>(this) + 0x350C);
    }

    UtlVector<matrix3x4>& getBoneCache()
    {
        return *reinterpret_cast<UtlVector<matrix3x4>*>(reinterpret_cast<uintptr_t>(this) + 0x2914);
    }

    uint32_t& getEffects() noexcept
    {
        static unsigned int m_fEffects = DataMap::findInDataMap(getPredDescMap(), "m_fEffects");
        return *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(this) + m_fEffects);
    }

    uint32_t& getIsPeeking() noexcept
    {
        static unsigned int m_bIsPeeking = DataMap::findInDataMap(getPredDescMap(), "m_bIsPeeking");
        return *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(this) + m_bIsPeeking);
    }

    uint32_t& getEFlags() noexcept
    {
        static unsigned int m_iEFlags = DataMap::findInDataMap(getPredDescMap(), "m_iEFlags");
        return *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(this) + m_iEFlags);
    }

    bool& useNewAnimationState() noexcept
    {
        return *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(this) + 0x26C5);
    }

    uint32_t& mostRecentModelBoneCounter() noexcept
    {
        static auto invalidateBoneCache = memory->invalidateBoneCache;
        static auto mostRecentModelBoneCounter = *reinterpret_cast<uintptr_t*>(invalidateBoneCache + 0x1B);

        return *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(this) + mostRecentModelBoneCounter);
    }

    float& lastBoneSetupTime() noexcept
    {
        static auto invalidateBoneCache = memory->invalidateBoneCache;
        static auto lastBoneSetupTime = *reinterpret_cast<uintptr_t*>(invalidateBoneCache + 0x11);

        return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + lastBoneSetupTime);
    }

    void invalidateBoneCache() noexcept
    {
        if (!this)
            return;

        lastBoneSetupTime() = -FLT_MAX;
        mostRecentModelBoneCounter() = UINT_MAX;
    }

    auto isKnife() noexcept { return getWeaponType() == WeaponType::Knife; }
    auto isPistol() noexcept { return getWeaponType() == WeaponType::Pistol; }
    auto isSniperRifle() noexcept { return getWeaponType() == WeaponType::SniperRifle; }
    auto isShotgun() noexcept { return getWeaponType() == WeaponType::Shotgun; }
    auto isGrenade() noexcept { return getWeaponType() == WeaponType::Grenade; }
    auto isBomb() noexcept { return getWeaponType() == WeaponType::C4; }

    bool isThrowing() noexcept
    {
        if (this->isGrenade())
        {
            if (!this->pinPulled())
            {
                float throwtime = this->throwTime();
                if (throwtime > 0)
                    return true;
            }
        }
        return false;
    }

    auto isFullAuto() noexcept
    {
        const auto weaponData = getWeaponData();
        if (weaponData)
            return weaponData->fullAuto;
        return false;
    }

    auto requiresRecoilControl() noexcept
    {
        const auto weaponData = getWeaponData();
        if (weaponData)
            return weaponData->recoilMagnitude < 35.0f && weaponData->recoveryTimeStand > weaponData->cycletime;
        return false;
    }

    bool unfixedSetupBones(matrix3x4* out, int maxBones, int boneMask, float currentTime) noexcept
    {
        return VirtualMethod::call<bool, 13>(this + sizeof(uintptr_t), out, maxBones, boneMask, currentTime);
    }

    bool setupBones(matrix3x4* out, int maxBones, int boneMask, float currentTime) noexcept
    {
        Vector absOrigin = getAbsOrigin();
        uintptr_t backupEffects = getEffects();

        this->invalidateBoneCache();
        getEffects() |= 8;

        memory->setAbsOrigin(this, origin());

        auto result = VirtualMethod::call<bool, 13>(this + sizeof(uintptr_t), out, maxBones, boneMask, currentTime);

        memory->setAbsOrigin(this, absOrigin);
        getEffects() = backupEffects;
        return result;
    }

    void getBonePos(int bone, Vector& origin) noexcept
    {
        Vector vectors[4];
        memory->getBonePos(this, bone, vectors);
        origin = { vectors[1].x, vectors[2].y, vectors[3].z };
    }

    Vector getBonePosition(int bone) noexcept
    {
        if (matrix3x4 boneMatrices[MAXSTUDIOBONES]; unfixedSetupBones(boneMatrices, MAXSTUDIOBONES, 256, 0.0f))
            return boneMatrices[bone].origin();
        else
            return Vector{ };
    }

    bool isVisible(const Vector& position = { }) noexcept
    {
        if (!localPlayer)
            return false;

        Trace trace;
        interfaces->engineTrace->traceRay({ localPlayer->getEyePosition(), position.notNull() ? position : getBonePosition(8) }, 0x46004009, { localPlayer.get() }, trace);
        return trace.entity == this || trace.fraction > 0.97f;
    }


    bool isOtherEnemy(Entity* other) noexcept;

    VarMap* getVarMap() noexcept
    {
        return reinterpret_cast<VarMap*>(this + 0x24);
    }
    NETVAR(getThrowStrength, "CBaseCSGrenade", "m_flThrowStrength", float)
        NETVAR(AddonBits, "CCSPlayer", "m_iAddonBits", int)
        NETVAR(HasGunGameImmunity, "CCSPlayer", "m_bGunGameImmunity", bool)
        NETVAR_OFFSET(getAnimstate, "CCSPlayer", "m_bIsScoped", -20, AnimState*)
        NETVAR(HealthBoostTime, "CCSPlayer", "m_flHealthShotBoostExpirationTime", float)
        NETVAR_OFFSET(projectileSpawnTime, "CBaseCSGrenadeProjectile", "m_vecExplodeEffectOrigin", +0xC, float)
        float getMaxDesyncAngle() noexcept
    {
        const auto animState = getAnimstate();

        if (!animState)
            return 0.0f;

        float yawModifier = (animState->walkToRunTransition * -0.3f - 0.2f) * std::clamp(animState->speedAsPortionOfWalkTopSpeed, 0.0f, 1.0f) + 1.0f;

        if (animState->animDuckAmount > 0.0f)
            yawModifier += (animState->animDuckAmount * std::clamp(animState->speedAsPortionOfCrouchTopSpeed, 0.0f, 1.0f) * (0.5f - yawModifier));

        return animState->aimYawMax * yawModifier;
    }

    bool isInReload() noexcept
    {
        return *reinterpret_cast<bool*>(uintptr_t(&clip()) + 0x41);
    }

    auto getUserId() noexcept
    {
        if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(index(), playerInfo))
            return playerInfo.userId;

        return -1;
    }

    PlayerInfo get_info() {
        PlayerInfo info;
        interfaces->engine->getPlayerInfo(index(), info);
        return info;
    }

    NETVAR(team, "CBaseEntity", "m_iTeamNum", Team)
        std::uint64_t getSteamId() noexcept
    {
        if (PlayerInfo playerInfo; interfaces->engine->getPlayerInfo(index(), playerInfo))
            return playerInfo.xuid;
        return 0;
    }

    void getPlayerName(char(&out)[128]) noexcept;
    [[nodiscard]] std::string getPlayerName() noexcept
    {
        char name[128];
        getPlayerName(name);
        return name;
    }

    bool canSee(Entity* other, const Vector& pos) noexcept;

    int animlayer_count();

    AnimationLayer* get_animlayers();


    bool visibleTo(Entity* other) noexcept;

    int& getButtons() noexcept
    {
        static auto buttons = DataMap::findInDataMap(getPredDescMap(), "m_nButtons");
        return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + buttons);
    }

    int& getButtonLast() noexcept
    {
        static auto buttonLast = DataMap::findInDataMap(getPredDescMap(), "m_afButtonLast");
        return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + buttonLast);
    }

    int& getButtonPressed() noexcept
    {
        static auto buttonPressed = DataMap::findInDataMap(getPredDescMap(), "m_afButtonPressed");
        return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + buttonPressed);
    }

    int& getButtonReleased() noexcept
    {
        static auto buttonReleased = DataMap::findInDataMap(getPredDescMap(), "m_afButtonReleased");
        return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + buttonReleased);
    }

    bool physicsRunThink(int thinkMethod = 0) noexcept
    {
        return memory->physicsRunThink(this, thinkMethod);
    }

    void updateButtonState(int userCmdButtonMask) noexcept
    {
        getButtonLast() = getButtons();

        getButtons() = userCmdButtonMask;
        const int buttonsChanged = getButtonLast() ^ getButtons();

        getButtonPressed() = buttonsChanged & getButtons();
        getButtonReleased() = buttonsChanged & (~getButtons());

    }

    void checkHasThinkFunction(bool isThinking = false) noexcept
    {
        return memory->checkHasThinkFunction(this, isThinking);
    }

    void runPreThink() noexcept
    {
        if (physicsRunThink())
            preThink();
    }

    void runThink() noexcept
    {
        const auto nextThinkTick = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0xFC);
        if (nextThinkTick != -1 &&
            nextThinkTick > 0 &&
            nextThinkTick <= tickBase())
        {
            *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0xFC) = -1;

            checkHasThinkFunction();
            think();
        }

    }
    NETVAR(useCustomAutoExposureMin, "CEnvTonemapController", "m_bUseCustomAutoExposureMin", bool)
        NETVAR(useCustomAutoExposureMax, "CEnvTonemapController", "m_bUseCustomAutoExposureMax", bool)
        NETVAR(useCustomBloomScale, "CEnvTonemapController", "m_bUseCustomBloomScale", bool)
        NETVAR(customAutoExposureMin, "CEnvTonemapController", "m_flCustomAutoExposureMin", float)
        NETVAR(customAutoExposureMax, "CEnvTonemapController", "m_flCustomAutoExposureMax", float)
        NETVAR(customBloomScale, "CEnvTonemapController", "m_flCustomBloomScale", float)
        NETVAR(rotation, "CBaseEntity", "m_angRotation", Vector)
        void runPostThink() noexcept
    {
        //We have to rebuild postThink because we dont want to mess with weapons
        interfaces->mdlCache->beginLock();

        if (isAlive())
        {
            updateCollisionBounds();

            if (flags() & 1)
                fallVelocity() = 0.0f;

            if (sequence() == -1)
                setSequence(0);

            studioFrameAdvance();
            memory->postThinkVPhysics(this);
        }

        memory->simulatePlayerSimulatedEntities(this);

        interfaces->mdlCache->endLock();
    }

    NETVAR_OFFSET(collisionChangeTime, "CCSPlayer", "m_bIsScoped", -0x50, float)

        NETVAR(didSmokeEffect, "CSmokeGrenadeProjectile", "m_bDidSmokeEffect", bool)
        NETVAR_OFFSET(eyeAnglesroll, "CCSPlayer", "m_angEyeAngles", 8, float)
        NETVAR(pinPulled, "CBaseCSGrenade", "m_bPinPulled", bool);
    NETVAR(throwTime, "CBaseCSGrenade", "m_fThrowTime", float_t);
    NETVAR(observerMode, "CBasePlayer", "m_iObserverMode", ObsMode)
        NETVAR(body, "CBaseAnimating", "m_nBody", int)
        NETVAR(clientSideAnimation, "CBaseAnimating", "m_bClientSideAnimation", bool)
        NETVAR(hitboxSet, "CBaseAnimating", "m_nHitboxSet", int)
        NETVAR(sequence, "CBaseAnimating", "m_nSequence", int)

        NETVAR(modelIndex, "CBaseEntity", "m_nModelIndex", unsigned)
        NETVAR(origin, "CBaseEntity", "m_vecOrigin", Vector)
        NETVAR_OFFSET(moveType, "CBaseEntity", "m_nRenderMode", 1, MoveType)
        NETVAR(simulationTime, "CBaseEntity", "m_flSimulationTime", float)
        NETVAR(ownerEntity, "CBaseEntity", "m_hOwnerEntity", int)
        NETVAR(spotted, "CBaseEntity", "m_bSpotted", bool)
        NETVAR(spottedByMask, "CBaseEntity", "m_bSpottedByMask", std::uint32_t)
        NETVAR_OFFSET(oldSimulationTime, "CBaseEntity", "m_flSimulationTime", 4, float)
        NETVAR(weapons, "CBaseCombatCharacter", "m_hMyWeapons", int[64])
        PNETVAR(wearables, "CBaseCombatCharacter", "m_hMyWearables", int)

        NETVAR(viewModel, "CBasePlayer", "m_hViewModel[0]", int)
        NETVAR(fov, "CBasePlayer", "m_iFOV", int)
        NETVAR(fovStart, "CBasePlayer", "m_iFOVStart", int)
        NETVAR(defaultFov, "CBasePlayer", "m_iDefaultFOV", int)
        NETVAR(flags, "CBasePlayer", "m_fFlags", int)
        NETVAR(tickBase, "CBasePlayer", "m_nTickBase", int)
        NETVAR(aimPunchAngle, "CBasePlayer", "m_aimPunchAngle", Vector)
        NETVAR(aimPunchAngleVelocity, "CBasePlayer", "m_aimPunchAngleVel", Vector)
        NETVAR(baseVelocity, "CBasePlayer", "m_vecBaseVelocity", Vector)
        NETVAR(viewPunchAngle, "CBasePlayer", "m_viewPunchAngle", Vector)
        NETVAR(viewOffset, "CBasePlayer", "m_vecViewOffset[0]", Vector)
        NETVAR(network_origin, "CBasePlayer", "m_vecNetworkOrigin", Vector)
        NETVAR(velocity, "CBasePlayer", "m_vecVelocity[0]", Vector)
        NETVAR(lastPlaceName, "CBasePlayer", "m_szLastPlaceName", char[18])
        NETVAR(getLadderNormal, "CBasePlayer", "m_vecLadderNormal", Vector)
        NETVAR(duckAmount, "CBasePlayer", "m_flDuckAmount", float)
        NETVAR(duckSpeed, "CBasePlayer", "m_flDuckSpeed", float)
        NETVAR(m_vecVelocity, "CBasePlayer", "m_vecVelocity[0]", Vector);
        NETVAR(m_vecViewOffset, "CBasePlayer", "m_vecViewOffset[0]", Vector);
        NETVAR(m_flThirdpersonRecoil, "CCSPlayer", "m_flThirdpersonRecoil", float);
        NETVAR(fallVelocity, "CBasePlayer", "m_flFallVelocity", float)
        NETVAR(m_fLastShotTime, "CWeaponCSBase", "m_fLastShotTime", float)
        NETVAR(armor, "CCSPlayer", "m_ArmorValue", int)
        NETVAR(hasHeavyArmor, "CCSPlayer", "m_bHasHeavyArmor", bool)
        NETVAR(eyeAngles, "CCSPlayer", "m_angEyeAngles", Vector)
        NETVAR(isScoped, "CCSPlayer", "m_bIsScoped", bool)
        NETVAR(isDefusing, "CCSPlayer", "m_bIsDefusing", bool)
        NETVAR(flashMaxAlpha, "CCSPlayer", "m_flFlashMaxAlpha", float)
        NETVAR_OFFSET(flashDuration, "CCSPlayer", "m_flFlashMaxAlpha", 4, float)
        NETVAR_OFFSET(flashBuildUp, "CCSPlayer", "m_flFlashMaxAlpha", -4, bool)
        NETVAR_OFFSET(flashOverlayAlpha, "CCSPlayer", "m_flFlashMaxAlpha", -8, float)
        NETVAR_OFFSET(flashScreenshotAlpha, "CCSPlayer", "m_flFlashMaxAlpha", -12, float)
        NETVAR_OFFSET(flashBangTime, "CCSPlayer", "m_flFlashMaxAlpha", -16, float)
        NETVAR(gunGameImmunity, "CCSPlayer", "m_bGunGameImmunity", bool)
        NETVAR(account, "CCSPlayer", "m_iAccount", int)
        NETVAR(inBombZone, "CCSPlayer", "m_bInBombZone", bool)
        NETVAR(hasDefuser, "CCSPlayer", "m_bHasDefuser", bool)
        NETVAR(hasHelmet, "CCSPlayer", "m_bHasHelmet", bool)
        NETVAR(lby, "CCSPlayer", "m_flLowerBodyYawTarget", float)
        NETVAR(ragdoll, "CCSPlayer", "m_hRagdoll", int)
        NETVAR(shotsFired, "CCSPlayer", "m_iShotsFired", int)
        NETVAR(money, "CCSPlayer", "m_iAccount", int)
        NETVAR(waitForNoAttack, "CCSPlayer", "m_bWaitForNoAttack", bool)
        NETVAR(isStrafing, "CCSPlayer", "m_bStrafing", bool)
        NETVAR(moveState, "CCSPlayer", "m_iMoveState", int)
        NETVAR(duckOverride, "CCSPlayer", "m_bDuckOverride", bool)
        NETVAR(stamina, "CCSPlayer", "m_flStamina", float)
        NETVAR(thirdPersonRecoil, "CCSPlayer", "m_flThirdpersonRecoil", float)
        NETVAR(velocityModifier, "CCSPlayer", "m_flVelocityModifier", float)

        NETVAR(GetModelScale, "CBaseAnimating", "m_flModelScale", float);

        NETVAR(viewModelIndex, "CBaseCombatWeapon", "m_iViewModelIndex", int)
        NETVAR(worldModelIndex, "CBaseCombatWeapon", "m_iWorldModelIndex", int)
        NETVAR(worldDroppedModelIndex, "CBaseCombatWeapon", "m_iWorldDroppedModelIndex", int)
        NETVAR(weaponWorldModel, "CBaseCombatWeapon", "m_hWeaponWorldModel", int)
        NETVAR(clip, "CBaseCombatWeapon", "m_iClip1", int)
        NETVAR(reserveAmmoCount, "CBaseCombatWeapon", "m_iPrimaryReserveAmmoCount", int)
        NETVAR(nextPrimaryAttack, "CBaseCombatWeapon", "m_flNextPrimaryAttack", float)
        NETVAR(nextSecondaryAttack, "CBaseCombatWeapon", "m_flNextSecondaryAttack", float)
        NETVAR(recoilIndex, "CBaseCombatWeapon", "m_flRecoilIndex", float)

        NETVAR(nextAttack, "CBaseCombatCharacter", "m_flNextAttack", float)

        NETVAR(readyTime, "CWeaponCSBase", "m_flPostponeFireReadyTime", float)
        NETVAR(burstMode, "CWeaponCSBase", "m_bBurstMode", bool)

        NETVAR(burstShotRemaining, "CWeaponCSBaseGun", "m_iBurstShotsRemaining", int)
        NETVAR(zoomLevel, "CWeaponCSBaseGun", "m_zoomLevel", int)

        NETVAR(accountID, "CBaseAttributableItem", "m_iAccountID", int)
        NETVAR(itemDefinitionIndex, "CBaseAttributableItem", "m_iItemDefinitionIndex", short)
        NETVAR(itemDefinitionIndex2, "CBaseAttributableItem", "m_iItemDefinitionIndex", WeaponId)
        NETVAR(itemIDHigh, "CBaseAttributableItem", "m_iItemIDHigh", int)
        NETVAR(entityQuality, "CBaseAttributableItem", "m_iEntityQuality", int)
        NETVAR(customName, "CBaseAttributableItem", "m_szCustomName", char[32])
        NETVAR(fallbackPaintKit, "CBaseAttributableItem", "m_nFallbackPaintKit", unsigned)
        NETVAR(fallbackSeed, "CBaseAttributableItem", "m_nFallbackSeed", unsigned)
        NETVAR(fallbackWear, "CBaseAttributableItem", "m_flFallbackWear", float)
        NETVAR(fallbackStatTrak, "CBaseAttributableItem", "m_nFallbackStatTrak", unsigned)
        NETVAR(initialized, "CBaseAttributableItem", "m_bInitialized", bool)

        NETVAR(owner, "CBaseViewModel", "m_hOwner", int)
        NETVAR(weapon, "CBaseViewModel", "m_hWeapon", int)

        NETVAR(c4StartedArming, "CC4", "m_bStartedArming", bool)

        NETVAR(tabletReceptionIsBlocked, "CTablet", "m_bTabletReceptionIsBlocked", bool)

        NETVAR(droneTarget, "CDrone", "m_hMoveToThisEntity", int)

        NETVAR(thrower, "CBaseGrenade", "m_hThrower", int)

        NETVAR(mapHasBombTarget, "CCSGameRulesProxy", "m_bMapHasBombTarget", bool)
        NETVAR(freezePeriod, "CCSGameRulesProxy", "m_bFreezePeriod", bool)
        NETVAR(isValveDS, "CCSGameRulesProxy", "m_bIsValveDS", bool)

        NETVAR(fireXDelta, "CInferno", "m_fireXDelta", int[100])
        NETVAR(fireYDelta, "CInferno", "m_fireYDelta", int[100])
        NETVAR(fireZDelta, "CInferno", "m_fireZDelta", int[100])
        NETVAR(fireIsBurning, "CInferno", "m_bFireIsBurning", bool[100])
        NETVAR(fireCount, "CInferno", "m_fireCount", int)

        float getFlashStartTime() noexcept
    {
        return (flashBangTime() - flashDuration());
    }
    float getFlashTimeElapsed() noexcept
    {
        return std::max<float>(memory->globalVars->currenttime - getFlashStartTime(), 0.0f);
    }

    bool isFlashed() noexcept
    {
        return flashOverlayAlpha() > 75.0f;
    }
};

class PlantedC4 : public Entity {
public:
    NETVAR(c4BlowTime, "CPlantedC4", "m_flC4Blow", float)
        NETVAR(c4TimerLength, "CPlantedC4", "m_flTimerLength", float)
        NETVAR(c4BombSite, "CPlantedC4", "m_nBombSite", int)
        NETVAR(c4Ticking, "CPlantedC4", "m_bBombTicking", bool)
        NETVAR(c4DefuseCountDown, "CPlantedC4", "m_flDefuseCountDown", float)
        NETVAR(c4DefuseLength, "CPlantedC4", "m_flDefuseLength", float)
        NETVAR(c4Defuser, "CPlantedC4", "m_hBombDefuser", int)
};

class FogController : public Entity {
public:
    NETVAR(enable, "CFogController", "m_fog.enable", int)
        NETVAR(start, "CFogController", "m_fog.start", float)
        NETVAR(end, "CFogController", "m_fog.end", float)
        NETVAR(density, "CFogController", "m_fog.maxdensity", float)
        NETVAR(color, "CFogController", "m_fog.colorPrimary", int)
};
