#include "AimbotFunctions.h"
#include "Animations.h"
#include "Resolver.h"

#include "../Logger.h"

#include "../SDK/GameEvent.h"
#include "../Vector2D.hpp"
#include <DirectXMath.h>
#include <algorithm>
#include "../xor.h"
#include "Backtrack.h"
#include "../GameData.h"
std::deque<Resolver::SnapShot> snapshots;
static std::array<Animations::Players, 65> players{};
std::deque<Resolver::Ticks> Resolver::bulletImpacts;
std::deque<Resolver::Ticks> Resolver::tick;
std::deque<Resolver::Tick> Resolver::shot[65];
std::string ResolverMode[65];
UserCmd* cmd;
bool resolver = true;
bool occlusion = false;
float desyncAng{ 0 };

#define TICK_INTERVAL            ( memory->globalVars->currenttime )
#define TIME_TO_TICKS( dt )        ( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )        ( TICK_INTERVAL *( t ) )
#define M_RADPI 57.295779513082f

void Resolver::initialize(Entity* e, adjust_data* record, const float& goal_feet_yaw, const float& pitch)
{
    player = e;
    player_record = record;

    original_goal_feet_yaw = Helpers::normalizeYaw(goal_feet_yaw);
    original_pitch = Helpers::normalize_pitch(pitch);
}

void Resolver::CmdGrabber(UserCmd* cmd1)
{
    cmd = cmd1;
}

void Resolver::reset()
{
    player = nullptr;
    player_record = nullptr;

    side = false;
    fake = false;

    was_first_bruteforce = false;
    was_second_bruteforce = false;

    ResolveSide[64] = 0;

    original_goal_feet_yaw = 0.0f;
    original_pitch = 0.0f;

    snapshots.clear();
}

void Resolver::saveRecord(int playerIndex, float playerSimulationTime) noexcept
{
    const auto entity = interfaces->entityList->getEntity(playerIndex);
    const auto player = Animations::getPlayer(playerIndex);
    if (!player.gotMatrix || !entity)
        return;

    SnapShot snapshot;
    snapshot.player = player;
    snapshot.playerIndex = playerIndex;
    snapshot.eyePosition = localPlayer->getEyePosition();
    snapshot.model = entity->getModel();

    if (player.simulationTime == playerSimulationTime)
    {
        snapshots.push_back(snapshot);
        return;
    }

    for (int i = 0; i < static_cast<int>(player.backtrackRecords.size()); i++)
    {
        if (player.backtrackRecords.at(i).simulationTime == playerSimulationTime)
        {
            snapshot.backtrackRecord = i;
            snapshots.push_back(snapshot);
            return;
        }
    }
}

Resolver::Ticks Resolver::getClosest(const float time) noexcept
{
    //works pretty good right now, no need to change it
    Resolver::Ticks record{ };
    record.position = Vector{};
    record.time = -1.0f;

    int bestTick = -1;
    float bestTime = FLT_MAX;

    for (size_t i = 0; i < tick.size(); i++)
    {
        if (tick.at(i).time > time)
            continue;

        float diff = time - tick.at(i).time;
        if (diff < 0.f)
            continue;

        if (diff < bestTime)
        {
            bestTime = diff;
            bestTick = i;
        }
    }
    if (bestTick == -1)
        return record;
    return tick[bestTick];
}

void Resolver::getEvent(GameEvent* event) noexcept
{
    if (!event || !localPlayer || interfaces->engine->isHLTV())
        return;

    switch (fnv::hashRuntime(event->getName())) {
    case fnv::hash("round_start"):
    {
        //Reset all
        auto players = Animations::setPlayers();
        if (players->empty())
            break;

        for (int i = 0; i < static_cast<int>(players->size()); i++)
        {
            players->at(i).misses = 0;
        }
        snapshots.clear();
        break;
    }
    case fnv::hash("weapon_fire"):
    {
        if (snapshots.empty())
            break;

        const auto playerId = event->getInt("userid");
        if (playerId == localPlayer->getUserId())
            break;

        const auto index = interfaces->engine->getPlayerFromUserID(playerId);
        Animations::setPlayer(index)->shot = true;
        break;
    }
    case fnv::hash("player_death"):
    {
        //Reset player
        if (snapshots.empty())
            break;

        //Reset player
        const auto playerId = event->getInt("userid");
        if (playerId == localPlayer->getUserId())
            break;

        const auto index = interfaces->engine->getPlayerFromUserID(playerId);
        Animations::setPlayer(index)->misses = 0;
        break;
    }
    case fnv::hash("player_hurt"):
    {
        if (snapshots.empty())
            break;
        if (!localPlayer || !localPlayer->isAlive())
        {
            snapshots.clear();
            return;
        }

        if (event->getInt("attacker") != localPlayer->getUserId())
            break;

        const auto hitgroup = event->getInt("hitgroup");
        if (hitgroup < HitGroup::Head || hitgroup > HitGroup::RightLeg)
            break;
        const auto index = interfaces->engine->getPlayerFromUserID(event->getInt("userid"));
        auto& snapshot = snapshots.front();
        if (desyncAng != 0.f)
        {
            if (hitgroup == HitGroup::Head)
            {
                Animations::setPlayer(index)->workingangle = desyncAng;
            }
        }
        const auto entity = interfaces->entityList->getEntity(snapshot.playerIndex);

        snapshots.pop_front(); //Hit somebody so dont calculate
        break;
    }
    case fnv::hash("bullet_impact"):
    {
        if (snapshots.empty())
            break;

        if (event->getInt("userid") != localPlayer->getUserId())
            break;

        auto& snapshot = snapshots.front();

        if (!snapshot.gotImpact)
        {
            snapshot.time = memory->globalVars->serverTime();
            snapshot.bulletImpact = Vector{ event->getFloat("x"), event->getFloat("y"), event->getFloat("z") };
            snapshot.gotImpact = true;
        }
        else
        {
            if (snapshot.player.shot)
            {
                Antionetap(event->getInt("userid"), interfaces->entityList->getEntity(snapshot.playerIndex), Vector{ event->getFloat("x"), event->getFloat("y"), event->getFloat("z") });
            }
        }
        break;
    }
    default:
        break;
    }
    if (!resolver)
        snapshots.clear();
}

Vector calcAngle(Vector source, Vector entityPos) {
    Vector delta = {};
    delta.x = source.x - entityPos.x;
    delta.y = source.y - entityPos.y;
    delta.z = source.z - entityPos.z;
    Vector angles = {};
    Vector viewangles = cmd->viewangles;
    angles.x = Helpers::rad2deg(atan(delta.z / hypot(delta.x, delta.y))) - viewangles.x;
    angles.y = Helpers::rad2deg(atan(delta.y / delta.x)) - viewangles.y;
    angles.z = 0;
    if (delta.x >= 0.f)
        angles.y += 180;

    return angles;
}

float get_backward_side(Entity* entity) {
    if (!entity->isAlive())
        return -1.f;
    const float result = Helpers::angleDiff(localPlayer->origin().y, entity->origin().y);
    return result;
}
float get_angle(Entity* entity) {
    return Helpers::angleNormalize(entity->eyeAngles().y);
}
float get_forward_yaw(Entity* entity) {
    return Helpers::angleNormalize(get_backward_side(entity) - 180.f);
}
float get_foword_yaw(Entity* entity) {
    return Helpers::angleNormalize(get_backward_side(entity) - 180.f);
}
float Resolver::GetLeftYaw(Entity* entity) {
    return Helpers::normalizeYaw(calcAngle(localPlayer->origin(), entity->origin()).y - 90.f);
}

float Resolver::GetRightYaw(Entity* entity) {
    return Helpers::normalizeYaw(calcAngle(localPlayer->origin(), entity->origin()).y + 90.f);
}

float build_server_abs_yaw(Entity* entity, const float angle)
{
    Vector velocity = entity->velocity();
    const auto& anim_state = entity->getAnimstate();
    const float m_fl_eye_yaw = angle;
    float m_fl_goal_feet_yaw = 0.f;

    const float eye_feet_delta = Helpers::angleDiff(m_fl_eye_yaw, m_fl_goal_feet_yaw);

    static auto get_smoothed_velocity = [](const float min_delta, const Vector a, const Vector b) {
        const Vector delta = a - b;
        const float delta_length = delta.length();

        if (delta_length <= min_delta)
        {
            if (-min_delta <= delta_length)
                return a;
            const float i_radius = 1.0f / (delta_length + FLT_EPSILON);
            return b - delta * i_radius * min_delta;
        }
        const float i_radius = 1.0f / (delta_length + FLT_EPSILON);
        return b + delta * i_radius * min_delta;
    };

    if (const float spd = velocity.squareLength(); spd > std::powf(1.2f * 260.0f, 2.f))
    {
        const Vector velocity_normalized = velocity.normalized();
        velocity = velocity_normalized * (1.2f * 260.0f);
    }

    const float m_fl_choked_time = anim_state->lastUpdateTime;
    const float duck_additional = std::clamp(entity->duckAmount() + anim_state->duckAdditional, 0.0f, 1.0f);
    const float duck_amount = anim_state->animDuckAmount;
    const float choked_time = m_fl_choked_time * 6.0f;
    float v28;

    // clamp
    if (duck_additional - duck_amount <= choked_time)
        if (duck_additional - duck_amount >= -choked_time)
            v28 = duck_additional;
        else
            v28 = duck_amount - choked_time;
    else
        v28 = duck_amount + choked_time;

    const float fl_duck_amount = std::clamp(v28, 0.0f, 1.0f);

    const Vector animation_velocity = get_smoothed_velocity(m_fl_choked_time * 2000.0f, velocity, entity->velocity());
    const float speed = std::fminf(animation_velocity.length(), 260.0f);

    float fl_max_movement_speed = 260.0f;

    if (Entity* p_weapon = entity->getActiveWeapon(); p_weapon && p_weapon->getWeaponData())
        fl_max_movement_speed = std::fmaxf(p_weapon->getWeaponData()->maxSpeed, 0.001f);

    float fl_running_speed = speed / (fl_max_movement_speed * 0.520f);
    float fl_ducking_speed = speed / (fl_max_movement_speed * 0.340f);
    fl_running_speed = std::clamp(fl_running_speed, 0.0f, 1.0f);

    float fl_yaw_modifier = (anim_state->walkToRunTransition * -0.3f - 0.2f) * fl_running_speed + 1.0f;

    if (fl_duck_amount > 0.0f)
    {
        float fl_ducking_speed2 = std::clamp(fl_ducking_speed, 0.0f, 1.0f);
        fl_yaw_modifier += (fl_ducking_speed2 * fl_duck_amount) * (0.5f - fl_yaw_modifier);
    }

    constexpr float v60 = -58.f;
    constexpr float v61 = 58.f;

    const float fl_min_yaw_modifier = v60 * fl_yaw_modifier;

    if (const float fl_max_yaw_modifier = v61 * fl_yaw_modifier; eye_feet_delta <= fl_max_yaw_modifier)
    {
        if (fl_min_yaw_modifier > eye_feet_delta)
            m_fl_goal_feet_yaw = fabs(fl_min_yaw_modifier) + m_fl_eye_yaw;
    }
    else
    {
        m_fl_goal_feet_yaw = m_fl_eye_yaw - fabs(fl_max_yaw_modifier);
    }

    Helpers::normalizeYaw(m_fl_goal_feet_yaw);

    if (speed > 0.1f || fabs(velocity.z) > 100.0f)
    {
        m_fl_goal_feet_yaw = Helpers::approachAngle(
            m_fl_eye_yaw,
            m_fl_goal_feet_yaw,
            (anim_state->walkToRunTransition * 20.0f + 30.0f)
            * m_fl_choked_time);
    }
    else
    {
        m_fl_goal_feet_yaw = Helpers::approachAngle(
            entity->lby(),
            m_fl_goal_feet_yaw,
            m_fl_choked_time * 100.0f);
    }

    return m_fl_goal_feet_yaw;
}

void Resolver::apply_side(Entity* entity, const int& choke)
{
    auto& info = resolver_info[player->index()];
    if (!entity->isAlive() || !info.resolved || info.side == side_original)
        return;

    auto state = player->getAnimstate();
    if (!state)
        return;

    float desync_angle = choke > 3 ? 120.f : entity->getMaxDesyncAngle();
    state->footYaw = Helpers::normalizeYaw(player->eyeAngles().y + desync_angle * info.side);
}

void Resolver::Antionetap(int userid, Entity* entity, Vector shot)
{
    std::vector<std::reference_wrapper<const PlayerData>> playersOrdered{ GameData::players().begin(), GameData::players().end() };
    std::ranges::sort(playersOrdered, [](const PlayerData& a, const PlayerData& b) {
        // enemies first
        if (a.enemy != b.enemy)
            return a.enemy && !b.enemy;

        return a.handle < b.handle;
        });
    for (const PlayerData& player : playersOrdered) {
        if (player.userId == userid)
        {
            if (entity->isAlive())
            {
                Vector pos = shot;
                Vector eyepos = entity->getEyePosition();
                Vector ang = calcAngle(eyepos, pos);
                Vector angToLocal = calcAngle(eyepos, localPlayer->getEyePosition());
                Vector delta = { angToLocal.x - ang.x, angToLocal.y - ang.y, 0 };
                float FOV = sqrt(delta.x * delta.x + delta.y * delta.y);
                if (FOV < 20.f)
                {
                    //Logger::addLog("[Medusa] " + player.name + " missed");
                    if (config->roll1)
                    {
                        config->roll1 = false;
                    }
                    else
                    {
                        config->roll1 = true;
                    }
                }
            }
        }
    }
}

void Resolver::detect_side(Entity* entity, int* side) {
    /* externals */
    Vector forward{};
    Vector right{};
    Vector up{};
    Trace tr;
    Helpers::AngleVectors(Vector(0, get_backward_side(entity), 0), &forward, &right, &up);
    /* filtering */

    const Vector src_3d = entity->getEyePosition();
    const Vector dst_3d = src_3d + forward * 384;

    /* back engine tracers */
    interfaces->engineTrace->traceRay({ src_3d, dst_3d }, MASK_SHOT, { entity }, tr);
    float back_two = (tr.endpos - tr.startpos).length();

    /* right engine tracers */
    interfaces->engineTrace->traceRay(Ray(src_3d + right * 35, dst_3d + right * 35), MASK_SHOT, { entity }, tr);
    float right_two = (tr.endpos - tr.startpos).length();

    /* left engine tracers */
    interfaces->engineTrace->traceRay(Ray(src_3d - right * 35, dst_3d - right * 35), MASK_SHOT, { entity }, tr);
    float left_two = (tr.endpos - tr.startpos).length();

    /* fix side */
    if (left_two > right_two) {
        *side = -1;
        //Body should be right
    }
    else if (right_two > left_two) {
        *side = 1;
    }
    else
        *side = 0;
}

void Resolver::delta_side_detect(int* side, Entity* entity)
{
    float EyeDelta = Helpers::normalizeYaw(entity->eyeAngles().y);

    if (fabs(EyeDelta) > 5)
    {
        if (EyeDelta > 5)
            *side = -1;
        else if (EyeDelta < -5)
            *side = 1;
    }
    else
        *side = 0;
}

bool DesyncDetect(Entity* entity)
{
    if (!localPlayer)
        return false;
    if (!entity || !entity->isAlive())
        return false;
    if (entity->isBot())
        return false;
    if (entity->team() == localPlayer->team())
        return false;
    if (entity->moveType() == MoveType::NOCLIP || entity->moveType() == MoveType::LADDER)
        return false;
    return true;
}

bool isSlowWalking(Entity* entity)
{
    float velocity_2D[64]{}, old_velocity_2D[64]{};
    if (entity->velocity().length2D() != velocity_2D[entity->index()] && entity->velocity().length2D() != NULL) {
        old_velocity_2D[entity->index()] = velocity_2D[entity->index()];
        velocity_2D[entity->index()] = entity->velocity().length2D();
    }
    Vector velocity = entity->velocity();
    Vector direction = entity->eyeAngles();

    float speed = velocity.length();
    direction.y = entity->eyeAngles().y - direction.y;
    //method 1
    if (velocity_2D[entity->index()] > 1) {
        int tick_counter[64]{};
        if (velocity_2D[entity->index()] == old_velocity_2D[entity->index()])
            tick_counter[entity->index()] += 1;
        else
            tick_counter[entity->index()] = 0;

        while (tick_counter[entity->index()] > (1 / memory->globalVars->intervalPerTick * fabsf(0.1f)))//should give use 100ms in ticks if their speed stays the same for that long they are definetely up to something..
            return true;
    }

    return false;
}

int GetChokedPackets(Entity* entity) {
    int last_ticks[65]{};
    auto ticks = timeToTicks(entity->simulationTime() - entity->oldSimulationTime());
    if (ticks == 0 && last_ticks[entity->index()] > 0) {
        return last_ticks[entity->index()] - 1;
    }
    else {
        last_ticks[entity->index()] = ticks;
        return ticks;
    }
}

bool didShoot(UserCmd* cmd) noexcept
{
    if (!(cmd->buttons & (UserCmd::IN_ATTACK)))
        return false;

    if (!localPlayer || localPlayer->nextAttack() > memory->globalVars->serverTime() || localPlayer->isDefusing() || localPlayer->waitForNoAttack())
        return false;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !activeWeapon->clip())
        return false;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return false;

    if (localPlayer->shotsFired() > 0 && !activeWeapon->isFullAuto())
        return false;

    Resolver::reset();

    return true;
}

bool freestand_target(Entity* target, float* yaw)
{
    float dmg_left = 0.f;
    float dmg_right = 0.f;

    static auto get_rotated_pos = [](Vector start, float rotation, float distance)
    {
        float rad = DEG2RAD(rotation);
        start.x += cos(rad) * distance;
        start.y += sin(rad) * distance;

        return start;
    };

    if (!localPlayer || !target || !localPlayer->isAlive())
        return false;

    Vector local_eye_pos = target->getEyePosition();
    Vector eye_pos = localPlayer->getEyePosition();
    Vector angle = (local_eye_pos, eye_pos);

    auto backwards = target->eyeAngles().y; // angle.y;

    Vector pos_left = get_rotated_pos(eye_pos, angle.y + 90.f, 60.f);
    Vector pos_right = get_rotated_pos(eye_pos, angle.y - 90.f, -60.f);

    const auto wall_left = (local_eye_pos, pos_left,
        nullptr, nullptr, localPlayer);

    const auto wall_right = (local_eye_pos, pos_right,
        nullptr, nullptr, localPlayer);

    if (dmg_left == 0.f && dmg_right == 0.f)
    {
        *yaw = backwards;
        return false;
    }

    // we can hit both sides, lets force backwards
    if (fabsf(dmg_left - dmg_right) < 5.f)
    {
        *yaw = backwards;
        return false;
    }

    bool direction = dmg_left > dmg_right;
    *yaw = direction ? angle.y - 90.f : angle.y + 90.f;

    return true;
}

float Resolver::max_desync_delta(Entity* player)
{
    auto animstate = player->getAnimstate();

    float duckammount = *(float*)(animstate + 0xA4);
    float speedfraction = std::fmax(0, std::fmin(*reinterpret_cast<float*>(animstate + 0xF8), 1));

    float speedfactor = std::fmax(0, std::fmin(1, *reinterpret_cast<float*> (animstate + 0xFC)));

    float unk1 = ((*reinterpret_cast<float*> (animstate + 0x11C) * -0.30000001) - 0.19999999) * speedfraction;
    float unk2 = unk1 + 1.1f;
    float unk3;

    if (duckammount > 0) {

        unk2 += ((duckammount * speedfactor) * (0.5 - unk2));

    }
    else
        unk2 += ((duckammount * speedfactor) * (0.5 - 0.58));

    unk3 = *(float*)(animstate + 0x334) * unk2;
    return unk3;
}

static auto resolve_update_animations(Entity* entity)
{
    Resolver::updating_animation = true;
    entity->updateClientSideAnimation();
    Resolver::updating_animation = false;
};
bool Resolver::DoesHaveJitter(Entity* player) {

    if (!player->isAlive())
        return false;

    if (!player->isDormant())
        return false;

    resolve_update_animations(player);

    static float LastAngle[64];
    static int LastBrute[64];
    static bool Switch[64];
    static float LastUpdateTime[64];

    int i = player->index();

    auto simulationtime = player->simulationTime();
    auto OLDsimulationtime = player->simulationTime() + 4;

    float CurrentAngle = player->eyeAngles().y;
    if (!Helpers::IsNearEqual(CurrentAngle, LastAngle[i], (max_desync_delta(player) - 4.f))) {
        Switch[i] = !Switch[i];
        LastAngle[i] = CurrentAngle;
        jitter_side = Switch[i] ? 1 : -1;
        LastBrute[i] = jitter_side;
        LastUpdateTime[i] = memory->globalVars->currenttime;
        return true;
    }
    else {
        if (fabsf(LastUpdateTime[i] - memory->globalVars->currenttime >= TICKS_TO_TIME(15))
            || simulationtime != OLDsimulationtime) {
            LastAngle[i] = CurrentAngle;
        }
        jitter_side = LastBrute[i];
    }
    return false;
}

float resolve_move_yaw(Animations::Players player, Entity* entity)
{
    float footYaw = Animations::getFootYaw();

    if (const auto eye_diff = Helpers::angleNormalize(Helpers::angleDiff(entity->eyeAngles().y, footYaw)); eye_diff > entity->getMaxDesyncAngle())
        player.rotation_side = ROTATION_SIDE_RIGHT;
    else if (entity->getMaxDesyncAngle() > eye_diff)
        player.rotation_side = ROTATION_SIDE_LEFT;
    else
        player.rotation_side = ROTATION_SIDE_CENTER;

    footYaw = Helpers::approachAngle(entity->eyeAngles().y, footYaw, (entity->getAnimstate()->walkToRunTransition * 20.0f + 30.0f) * entity->getAnimstate()->lastUpdateIncrement);

    player.rotation_side = player.last_side;
    return footYaw;
}

float build_move_yaw(Entity* entity)
{
    /* m_PlayerAnimationStateCSGO( )->m_flMoveYawIdeal */
    float moveYawIdeal = Animations::getFootYaw();

    /* Rebuild m_flMoveYawIdeal */
    float velocityLengthXY = entity->velocity().length2D();
    if (velocityLengthXY > 0 && entity->flags() & FL_ONGROUND)
    {
        /* convert horizontal velocity vec to angular yaw*/
        float rawYawIdeal = (atan2(-entity->velocity()[1], -entity->velocity()[0]) * M_RADPI);
        if (rawYawIdeal < 0.0f)
            rawYawIdeal += 180.0f;

        moveYawIdeal = build_server_abs_yaw(entity, Helpers::angleNormalize(Helpers::angleDiff(rawYawIdeal, Animations::getFootYaw())));
    }

    /* Return m_flMoveYaw */
    return moveYawIdeal;
}

void Resolver::MovingLayers()
{
    resolve_update_animations(player);

    int idx = player->index();

    float move_delta = 0.f;
    const auto delta_positive = fabsf(player_record->layers[ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate - player_record->resolver_layers[ROTATE_RIGHT][ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate);
    const auto delta_negative = fabsf(player_record->layers[ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate - player_record->resolver_layers[ROTATE_LEFT][ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate);
    const auto delta_eye = fabsf(player_record->layers[ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate - player_record->resolver_layers[ROTATE_SERVER][ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate);

    if (move_delta <= delta_eye)
        move_delta = delta_eye;
    else if (move_delta > delta_eye)
        move_delta = delta_negative;
    else
        move_delta = delta_positive;

    if (!(move_delta * 10000.0f) || (delta_eye * 10000.0f) != (delta_negative * 10000.0f)) {
        if (move_delta == delta_negative) {
            ResolveSide[idx] = -1;
        }
        else {
            ResolveSide[idx] = 1;
        }
    }
}

void Resolver::StoreAntifreestand()
{
    Vector forward{};
    Vector right{};
    Vector up{};
    Trace tr;
    auto animstate = player->getAnimstate();

    float LeftSide = animstate->eyeYaw - 60.0f;
    float RightSide = animstate->eyeYaw + 60.0f;

    Vector m_vecDirectionLeft, m_vecDirectionRight;
    Vector m_vecDesyncLeft(0.0f, LeftSide, 0.0f);
    Vector m_vecDesyncRight(0.0f, RightSide, 0.0f);

    Helpers::AngleVectors(m_vecDesyncLeft, &m_vecDirectionRight, &forward, &up);
    Helpers::AngleVectors(m_vecDesyncRight, &m_vecDirectionLeft, &forward, &up);

    const auto m_vecSrc = LocalPlayer()->getEyePosition();
    const auto m_vecLeftSrc = m_vecSrc + (m_vecDesyncLeft * 8192.f);
    const auto m_vecRightSrc = m_vecSrc + (m_vecDesyncRight * 8192.f);

    interfaces->engineTrace->traceRay(Ray(m_vecSrc, m_vecLeftSrc), MASK_SHOT, { player }, tr);
    float left_two = (tr.endpos - tr.startpos).length();

    interfaces->engineTrace->traceRay(Ray(m_vecSrc, m_vecLeftSrc), MASK_SHOT, { player }, tr);
    float right_two = (tr.endpos - tr.startpos).length();

    if (left_two > right_two)
        FreestandSide[player->index()] = 1;
    else if (left_two < right_two)
        FreestandSide[player->index()] = -1;
    else
        FreestandSide[player->index()] = 0;
}

bool ValidPitch(Entity* entity)
{
    int pitch = entity->eyeAngles().x;
    return pitch == 0 || pitch > 90 || pitch < -90;
}

void Resolver::ResolveStand()
{
    int idx = player->index();
    resolve_update_animations(player);
}

float flAngleMod(float flAngle)
{
    return((360.0f / 65536.0f) * ((int32_t)(flAngle * (65536.0f / 360.0f)) & 65535));
}

float ApproachAngle(float flTarget, float flValue, float flSpeed)
{
    flTarget = flAngleMod(flTarget);
    flValue = flAngleMod(flValue);

    float delta = flTarget - flValue;

    if (flSpeed < 0)
        flSpeed = -flSpeed;

    if (delta < -180)
        delta += 360;
    else if (delta > 180)
        delta -= 360;

    if (delta > flSpeed)
        flValue += flSpeed;
    else if (delta < -flSpeed)
        flValue -= flSpeed;
    else
        flValue = flTarget;

    return flValue;
}

void Resolver::ApplyAngle(Entity* entity) {
    auto resolver_info = m_info[entity->index()];

    if (resolver_info.m_valid)
        return;

    auto state = player->getAnimstate();
    if (!state)
        return;

    // apply resolved angle
    float angle = resolver_info.m_dsyangle * resolver_info.m_side;
    state->eyeYaw = Helpers::angleNormalize((entity->eyeAngles().y + angle));

    // force roll
    if (resolver_info.m_roll)
        entity->eyeAngles().z = Helpers::angleNormalize(50.f * -resolver_info.m_side);
}

//Rip legit aa resolving, todo: make a fix for this.
//Gheto but works good if the target does not know you have this LOL.
bool Resolver::DoesHaveFakeAngles(Entity* entity) {
    if (entity->eyeAngles().x > 0.f && entity->eyeAngles().x < 85.f)
        return false;

    if (entity->eyeAngles().x < 0.f && entity->eyeAngles().x > -85.f)
        return false;
}

//This is not always accurate so make sure to improve it in the future so we get more accurate results.
bool Resolver::GetLowDeltaState(Entity* entity) {
    auto animstate = entity->getAnimstate();

    float fl_eye_yaw = entity->eyeAngles().y;
    float fl_desync_delta = remainderf(fl_eye_yaw, animstate->footYaw);
    fl_desync_delta = std::clamp(fl_desync_delta, -60.f, 60.f);

    if (fabs(fl_desync_delta) < 30.f)
        return true;

    return false;
}

void Resolver::ResolveAir()
{
    resolve_update_animations(player);

    int i = player->index();
}

void Resolver::ResolveJitter()
{
    auto animstate = player->getAnimstate();
    auto jitter_first_side = player_record->Right;
    auto jitter_second_side = player_record->Left;
}

//This function just bruteforces where there hitbox will be so we can hit it. (duh)
float Resolver::BruteForce(Entity* entity, bool roll) {
    auto animstate = player->getAnimstate();

    auto idx = entity->index();
    auto missed_shot = missed_shots[idx];

    float delta = 60.f;
    if (!roll) {
        switch (missed_shot % 6) {
        case 3:
            delta = 0.f;
            break;
        case 4:
            delta = 58.f;
            break;
        case 5:
            delta = -58.f;
            break;
        case 6:
            delta = 29.f;
            break;
        case 7:
            delta = -29.f;
            break;
        default:
            delta = 0.f;
            break;
        }
    }
    else {
        switch (missed_shot % 6) {
        case 3:
            delta = 50.f;
            break;
        case 4:
            delta = -50.f;
            break;
        case 5:
            delta = 25.f;
            break;
        case 6:
            delta = -25.5f;
            break;
        case 7:
            delta = 0.f;
            break;
        default:
            delta = 0.f;
            break;
        }
    }

    return delta;
}

float GetBackwardYaw(Entity* entity) { return Helpers::calculate_angle(localPlayer->getAbsOrigin(), entity->getAbsOrigin()).y; }
int Resolver::detect_freestanding(Entity* entity)
{
    Vector src3D, dst3D, forward, right, up, src, dst;
    float back_two, right_two, left_two;
    Trace tr;

    Helpers::AngleVectors(Vector(0, GetBackwardYaw(entity), 0), &forward, &right, &up);

    src3D = player->getEyePosition();
    dst3D = src3D + (forward * 380);

    interfaces->engineTrace->traceRay({ src3D, dst3D }, MASK_SHOT, { entity }, tr);
    back_two = (tr.endpos - tr.startpos).Length();

    interfaces->engineTrace->traceRay(Ray(src3D + right * 35, dst3D + right * 35), MASK_SHOT, { entity }, tr);
    right_two = (tr.endpos - tr.startpos).Length();

    interfaces->engineTrace->traceRay(Ray(src3D - right * 35, dst3D - right * 35), MASK_SHOT, { entity }, tr);
    left_two = (tr.endpos - tr.startpos).Length();

    if (left_two > right_two)
        return -1;
    else if (right_two > left_two)
        return 1;
    else
        return 0;
}

_NODISCARD static constexpr float(max1)() noexcept {
    return FLT_MAX;
}
float Resolver::GetAwayAngle(adjust_data* record) {
    float  delta{ max1() };
    vec_t pos;
    ang_t  away;
    Vector forward, right, up;

    //if (g_cl.m_net_pos.empty()) {
    Helpers::AngleVectors(localPlayer->origin() - record->m_pred_origin, &forward, &right, &up);
    return away.y;
    //}

    //float owd = (g_cl.m_latency / 2.f);

    //float target = record->m_pred_time;

    // iterate all.
    //for (const auto& net : g_cl.m_net_pos) {
    //	float dt = std::abs(target - net.m_time);

        // the best origin.
    //	if (dt < delta) {B
    //		delta = dt;
    //		pos = net.m_pos;
    //	}
    //}

    //math::VectorAngles(pos - record->m_pred_origin, away);
    //return away.y;
}

bool Resolver::IsYawSideways(Entity* entity, float yaw)
{
    auto local_player = localPlayer;
    if (!local_player)
        return false;

    const auto at_target_yaw = Helpers::calculate_angle(local_player->origin(), entity->origin()).y;
    const float delta = fabs(Helpers::normalizeYaw(at_target_yaw - yaw));

    return delta > 20.f && delta < 160.f;
}

// pi constants.
constexpr float pi = 3.1415926535897932384f; // pi
constexpr float pi_2 = pi * 2.f;               // pi * 2
// radians to degrees.
__forceinline constexpr float rad_to_deg(float val) {
    return val * (180.f / pi);
}
void Resolver::ResolveAir1(adjust_data* data, Entity* entity) {
    // for no-spread call a seperate resolver.
   // if (g_menu.main.config.mode.get() == 1) {
  //      AirNS(data, record);
  //      return;
  //  }

    // else run our matchmaking air resolver.

    // we have barely any speed. 
    // either we jumped in place or we just left the ground.
    // or someone is trying to fool our resolver.
    if (entity->velocity().length2D() < 60.f) {
        // set this for completion.
        // so the shot parsing wont pick the hits / misses up.
        // and process them wrongly.
        m_mode = Modes::RESOLVE_STAND;

        // invoke our stand resolver.
        ResolveStand(); // later resolvestand 1

        // we are done.
        return;
    }

    // try to predict the direction of the player based on his velocity direction.
    // this should be a rough estimation of where he is looking.
    float velyaw = rad_to_deg(std::atan2(data->m_velocity.y, data->m_velocity.x));

    switch (data->m_shots % 3) {
    case 0:
        entity->eyeAngles().y = velyaw + 180.f;
        break;

    case 1:
        entity->eyeAngles().y = velyaw - 90.f;
        break;

    case 2:
        entity->eyeAngles().y = velyaw + 90.f;
        break;
    }
}

void Resolver::ResolveWalk(adjust_data* data, Entity* entity) {
    // apply lby to eyeangles.
    entity->eyeAngles().y = data->m_body;

    // reset stand and body index.
    data->m_body_index = 0;
    data->m_stand_index = 0;
    data->m_stand_index1 = 0;
    data->m_reverse_fs = 0;
    data->m_stand_index3 = 0;
    data->m_last_move = 0;
    data->m_has_body_updated = false;

    // copy the last record that this player was walking
    // we need it later on because it gives us crucial data.
    std::memcpy(&data->m_walk_record, entity, sizeof(adjust_data));
}

void Resolver::SetMode(adjust_data* record) {
    // the resolver has 3 modes to chose from.
    // these modes will vary more under the hood depending on what data we have about the player
    // and what kind of hack vs. hack we are playing (mm/nospread).

    float speed = record->m_anim_velocity.length();

    // if on ground, moving, and not fakewalking.
    if ((record->m_flags & FL_ONGROUND) && speed > 0.1f && !record->m_fake_walk)
        record->m_mode = Modes::RESOLVE_WALK;

    // if on ground, not moving or fakewalking.
    if ((record->m_flags & FL_ONGROUND) && (speed <= 0.1f || record->m_fake_walk))
        record->m_mode = Modes::RESOLVE_STAND;

    // if not on ground.
    else if (!(record->m_flags & FL_ONGROUND))
        record->m_mode = Modes::RESOLVE_AIR;
}

void Resolver::MatchShot(adjust_data* data, Entity* entity) {
    // do not attempt to do this in nospread mode.
   // if (g_menu.main.config.mode.get() == 1)
   //     return;

    float shoot_time = -1.f;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (activeWeapon) {
        // with logging this time was always one tick behind.
        // so add one tick to the last shoot time.
        shoot_time = activeWeapon->m_fLastShotTime() + memory->globalVars->intervalPerTick;
    }

    // this record has a shot on it.
    if (TIME_TO_TICKS(shoot_time) == TIME_TO_TICKS(entity->simulationTime())) {
        if (data->m_lag <= 2)
            data->m_shot = true;

        // more then 1 choke, cant hit pitch, apply prev pitch.
        else if (data->m_records.size() >= 2) {
            adjust_data* previous = data->m_records[1].get();

            if (previous && !previous->dormant())
                data->m_eye_angles.x = previous->m_eye_angles.x;
        }
    }
}

void Resolver::resolve_entity(Animations::Players& player, Animations::Players prev_player, Entity* entity) {

    // ignore bots
    if (entity->isBot())
        return;

    // get the players max rotation.
    if (DesyncDetect(entity) == false)
        return;

    if (!entity || !entity->getAnimstate())
        return;

    if (detect_freestanding(entity) == -1 || detect_freestanding(entity) == 1)
        return;

    // mark this record if it contains a shot.
    MatchShot(player_record -1, entity);

    // next up mark this record with a resolver mode that will be used.
    SetMode(player_record -1);

    // force them down.
    entity->eyeAngles().x = 89.f;

    // we arrived here we can do the acutal resolve.
    if (m_mode == Modes::RESOLVE_WALK)
        ResolveWalk(player_record, entity);

    else if (m_mode == Modes::RESOLVE_AIR)
        ResolveAir1(player_record, entity);

    const int iEntityID = entity->index();
    static float flFakePitch[65];

    if (fabsf(entity->getAnimstate()->eyePitch) == 180.f)
        flFakePitch[iEntityID] = entity->getAnimstate()->eyePitch;
    else if (bDidShot) flFakePitch[iEntityID] = NULL;

    if (fabsf(flFakePitch[iEntityID]) == 180.f)
        entity->eyeAngles() = Vector(89.f, entity->getAnimstate()->eyeYaw, 0.f);

    auto& resolver_info = m_info[entity->index()];
    resolver_info.m_valid = false;
    auto animstate = entity->getAnimstate();

    // there is history resolver, grab info from it
    // TO-DO: disable history when resolved side changes
    auto& history = m_history[entity->index()];
    if (!history.empty()) {
        // grab latest resolver data
        auto& history_resolver = history.front();

        resolver_info.m_history = true;
        resolver_info.m_side = history_resolver.m_side;
        resolver_info.m_mode = history_resolver.m_mode;
        resolver_info.m_dsyangle = history_resolver.m_dsyangle;
        return;
    }

    resolver_history res_history = HISTORY_UNKNOWN;
    resolver_type type = resolver_type(-1);

    bool is_player_zero = false;

    if (res_history == HISTORY_ZERO)
        is_player_zero = true;

    auto choked = abs(TIME_TO_TICKS(entity->simulationTime() - entity->oldSimulationTime()) - 1);
    bool is_player_faking = false;
    if (fabs(original_pitch) > 65.f || choked >= 1 || is_player_faking)
        fake = true;;

    Resolver::ApplyAngle(entity);

    ///////////////////// [ ANIMLAYERS ] /////////////////////
    auto i = entity->index();
    AnimationLayer layers[13];
    memcpy(layers, entity->get_animlayers(), entity->animlayer_count() * sizeof(AnimationLayer));

    Vector velocity = entity->m_vecVelocity();
    float spd = entity->velocity().length2D();
    if (spd > std::powf(1.2f * 260.0f, 2.f)) {
        Vector velocity_normalized = velocity.Normalized();
        velocity = velocity_normalized * (1.2f * 260.0f);
    }

    int idx = entity->index();

    const auto slow_walking = animstate->footYaw >= 0.01f && animstate->footYaw <= 0.8f;

    if (slow_walking)
    {
        if (animstate->footYaw != animstate->footYaw)
            animstate->footYaw = animstate->footYaw * side;
    }

    /* vars */
    float footYaw = Animations::getFootYaw();

    float max_rotation = entity->getMaxDesyncAngle();
    int index = 0;
    const float eye_yaw = entity->getAnimstate()->eyeYaw;

    bool OnAir = entity->flags() & FL_ONGROUND && !entity->getAnimstate()->landedOnGroundThisFrame;
    bool Ducking = entity->getAnimstate()->animDuckAmount && entity->flags() & FL_ONGROUND && !entity->getAnimstate()->landedOnGroundThisFrame;
    float Speed = entity->velocity().length2D();
    auto valid_lby = true;

    if (!animstate && choked == 0 || !animstate, choked == 0)
        return;

    float lby = entity->lby();
    bool crouching = entity->flags() & FL_ONGROUND && !entity->getAnimstate()->landedOnGroundThisFrame;
    if (crouching)
        entity->getAnimstate()->footYaw = Helpers::normalizeYaw(entity->eyeAngles().y + lby);

    Resolver::apply_side(entity, choked);

    //RIFK RESOLVER
    const auto missed_shots1 = missed_shots[entity->index()];
    auto logic_resolver_1 = [&]() // rifk7 resolver with changes
    {
        const auto local = LocalPlayer();

        if (!localPlayer)
            return 0.f;

        auto resolver_yaw = 0.f;
        auto freestanding_yaw = 0.f;

        const auto current_shot = missed_shots1 % 2;
        float max_desync_angle = 29.f;

        const auto plus_desync = entity->eyeAngles().y + max_desync_angle;
        const auto minus_desync = entity->eyeAngles().y - max_desync_angle;

        StoreAntifreestand();

        const auto diff_from_plus_desync = fabs(Helpers::angleDiff(freestanding_yaw, plus_desync));
        const auto diff_from_minus_desync = fabs(Helpers::angleDiff(freestanding_yaw, minus_desync));

        const auto first_yaw = diff_from_plus_desync < diff_from_minus_desync ? plus_desync : minus_desync;
        const auto second_yaw = diff_from_plus_desync < diff_from_minus_desync ? minus_desync : plus_desync;
        const auto third_yaw = Helpers::calculate_angle(localPlayer->getEyePosition(), entity->getEyePosition()).y;

        switch (current_shot)
        {
        case 0:
            resolver_yaw = first_yaw;
            break;
        case 1:
            resolver_yaw = second_yaw;
            break;
        case 2:
            resolver_yaw = third_yaw;
            break;
        default:
            break;
        }

        return resolver_yaw;
    };
    //END RIFK RESOLVER

    Resolver::detect_side(entity, &player.side);
    Resolver::setup_detect(player, entity);//setup tha detections

    if (freestand_target)
    {
        footYaw = Helpers::normalizeYaw((entity->eyeAngles().y + 90.f) * side);
    }

    UpdateShots(cmd);

    float angle = get_angle(entity);
    int new_side = 0;
    if (DoesHaveJitter(entity) && entity->eyeAngles().x < 45) {
        switch (missed_shots[idx] % 2) {
        case 0:
            ResolverMode[idx];
            animstate->footYaw = Helpers::normalizeYaw(angle + 58.f * new_side);
            break;
        case 1:
            ResolverMode[idx];
            animstate->footYaw = Helpers::normalizeYaw(angle - 58.f * new_side);
            break;
        }
    }

    bool IsLowDelta = GetLowDeltaState(entity);
    const bool& Sideways = fabsf(Helpers::normalizeYaw((angle - GetLeftYaw(entity))) < 45.f || fabsf(Helpers::normalizeYaw(angle - GetRightYaw(entity))) < 45.f); //Use this for sideways check.
    auto missed_shot = missed_shots[idx];
    //Simple fix to over max brute fix.
    if (missed_shot == 8 || missed_shot > 7) {
        missed_shot = 0;
        return;
    }
    //Resolve desync.
    if (DoesHaveFakeAngles(entity)) {
        if (missed_shot > 2 || missed_shot == 2) {
            desync_angle = BruteForce(entity, false);
            goto Skip_logic;
        }
        else if (Sideways) {
            should_force_safepoint = true;
            goto Skipped;
        }
        else {
            should_force_safepoint = false;

            if (detect_side)
                desync_angle = !IsLowDelta ? 58 : 29;
            else
                desync_angle = !IsLowDelta ? -58 : -29;
        }

    }
Skip_logic:
    //Set our goal feet yaw and we now hit p100.
    animstate->footYaw = Helpers::angleNormalize(entity->eyeAngles().y + desync_angle);
Skipped:
    //Set globals
    desync = desync_angle;

    //Set our pitch to the original players.
    //todo: Add a no spread resolver 💀 :skull:
    entity->eyeAngles().x = original_pitch;

    resolve_update_animations(entity);

    ValidPitch(entity);

    if (!player.extended && fabs(max_rotation) > 58.f)
    {
        max_rotation = max_rotation / 1.8f;
    }

    // resolve shooting players separately.
    if (player.shot) {
        entity->getAnimstate()->footYaw = eye_yaw + resolve_shot(player, entity);
        return;
    }
    if (entity->velocity().length2D() <= 0.1f) {
        const float angle_difference = Helpers::angleDiff(eye_yaw, entity->getAnimstate()->footYaw);
        index = 2 * angle_difference <= 0.0f ? 1 : -1;
    }
    else
    {
        if (!((int)player.layers[12].weight * 1000.f) && entity->velocity().length2D() > 0.1) {

            auto m_layer_delta1 = abs(player.layers[6].playbackRate - player.oldlayers[6].playbackRate);
            auto m_layer_delta2 = abs(player.layers[6].playbackRate - player.oldlayers[6].playbackRate);
            auto m_layer_delta3 = abs(player.layers[6].playbackRate - player.oldlayers[6].playbackRate);

            if (m_layer_delta1 < m_layer_delta2
                || m_layer_delta3 <= m_layer_delta2
                || (signed int)(float)(m_layer_delta2 * 1000.0))
            {
                if (m_layer_delta1 >= m_layer_delta3
                    && m_layer_delta2 > m_layer_delta3
                    && !(signed int)(float)(m_layer_delta3 * 1000.0))
                {
                    index = 1;
                }
            }
            else
            {
                index = -1;
            }
        }
    }
    
    if (!animstate)
    {
        player_record->side = RESOLVER_ORIGINAL;
        return;
    }

    if (entity->getAnimstate()->velocityLengthXY > 0.1f && fabsf(entity->getAnimstate()->velocityLengthZ) > 100.0f)
    {

        player.anim_resolved = false;

        int m_side = 0;
        bool m_can_animate;

        /* animlayers.*/
        /* check if we have a previous record stored we can compare to, if we are currently running animations related to body lean and check if the movement speed (movement layer playback rate) of both records match. */
        if (!(player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].weight * 1000.0f))
        {
            if (player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].weight * 1000.0f == prev_player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].weight * 1000.0f)
            {
                /* compare networked layer to processed layer w / m_flFootYaw lerped with positive delta. */
                const float delta_left = player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate;

                /* compare networked layer to processed layer. */
                const float delta_center = player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate;

                /* compare networked layer to processed layer. quad */
                const float delta_quad = player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate;

                /* compare networked layer to processed layer w/ m_flFootYaw lerped with negative delta. */
                const float delta_right = player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate;

                bool m_finally_active{};
                if (!(delta_quad * 1000.f))
                    m_finally_active = true;

                m_side = 0;

                float last_delta = abs(player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].playbackRate - delta_quad);

                if (delta_center * 1000.f || delta_quad < delta_center)
                    m_can_animate = m_finally_active;
                else
                {
                    player.rotation_mode = ROTATION_MOVE; /* record is mode resolve. */
                    m_can_animate = true;
                    m_side = ROTATION_SIDE_CENTER; /* record is desync. */
                    last_delta = delta_center;
                }

                if (!player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].weight)
                    player.rotation_mode = ROTATION_MOVE;
                m_can_animate = true;
                m_side = ROTATION_SIDE_LEFT;
                last_delta = delta_center;

                if (!(delta_left * 1000.f) && last_delta >= delta_left)
                {
                    m_can_animate = true;
                    m_side = ROTATION_SIDE_RIGHT; /* record is desync. */
                    last_delta = delta_left;
                }

                if (!(delta_right * 1000.f) && last_delta >= delta_right)
                {
                    m_can_animate = true;
                    m_side = ROTATION_SIDE_LEFT; /* record is desync. */
                    return;
                }
            }
            else
            {
                m_can_animate = false;

            }
        }

        entity->getAnimstate()->setupVelocity(); // setup vel

        footYaw = resolve_shot(player, entity);
        footYaw = resolve_move_yaw(player, entity);
        footYaw = build_move_yaw(entity);

        /* last hit. */
        player.anim_resolved = m_can_animate;
        player.rotation_side = m_side;

        switch (player.misses % 3) {
        case 0: //default
            entity->getAnimstate()->footYaw = build_server_abs_yaw(entity, entity->eyeAngles().y + max_rotation * static_cast<float>(index));
            break;
        case 1: //reverse
            entity->getAnimstate()->footYaw = build_server_abs_yaw(entity, entity->eyeAngles().y + max_rotation * static_cast<float>(-index));
            break;
        case 2: //middle
            entity->getAnimstate()->footYaw = build_server_abs_yaw(entity, entity->eyeAngles().y);
            break;
        default: break;
        }

    }

    // normalize the eye angles, doesn't really matter but its clean.
    Helpers::normalizeYaw(entity->eyeAngles().y);
}

bool IsAdjustingBalance(const Animations::Players& player, Entity* entity)
{
    for (int i = 0; i < 15; i++)
    {
        const int activity = entity->sequence();
        if (activity == 979)
        {
            return true;
        }
    }
    return false;
}

bool is_breaking_lby(const Animations::Players& player, Entity* entity, AnimationLayer cur_layer, AnimationLayer prev_layer)
{
    if (IsAdjustingBalance(player, entity))
    {
        if ((prev_layer.cycle != cur_layer.cycle) || cur_layer.weight == 1.f)
        {
            return true;
        }
        else if (cur_layer.cycle == 0.f && (prev_layer.cycle > 0.92f && cur_layer.cycle > 0.92f))
        {
            return true;
        }
    }

    return false;
}

float Resolver::resolve_shot(const Animations::Players& player, Entity* entity)
{
    const float fl_pseudo_fire_yaw = Helpers::angleNormalize(Helpers::angleDiff(localPlayer->origin().y, player.matrix[8].origin().y));

    float max_rotation = entity->lby();
    float resolve_value = entity->getMaxDesyncAngle();
    const int m_missed_shots = player.misses;

    float yaw_resolve = 0.f;

    /* miss & dsy val */
    if (m_missed_shots > 0)
        resolve_value *= -1;

    /* side layers */
    if (player.rotation_side != 0)
        resolve_value *= player.rotation_side; // Adjust the desync based on the rot side

    /* clamp angle */
    if (max_rotation < resolve_value)
        resolve_value = max_rotation;

    /* detect if entity is using max desync angle */
    yaw_resolve = resolve_value = max_rotation;

    /* vars side */
    const float fl_left_fire_yaw_delta = fabsf(Helpers::angleNormalize(fl_pseudo_fire_yaw - (entity->eyeAngles().y + ROTATION_SIDE_LEFT)));
    const float fl_right_fire_yaw_delta = fabsf(Helpers::angleNormalize(fl_pseudo_fire_yaw - (entity->eyeAngles().y - ROTATION_SIDE_RIGHT)));

    /* vars yaw */
    const auto side_res = player.rotation_side ? fl_left_fire_yaw_delta : fl_right_fire_yaw_delta;
    const float is_jitter = abs(fl_pseudo_fire_yaw - entity->eyeAngles().y) > entity->getMaxDesyncAngle();
    float velocityLengthXY = entity->velocity().length2D();
    const float stand = abs(velocityLengthXY < 260.0f);
    const float move = (!player.layers[ANIMATION_LAYER_MOVEMENT_MOVE].weight, velocityLengthXY > 260.0f);
    if (velocityLengthXY > 0 && entity->flags() & FL_ONGROUND)
    {
        // convert horizontal velocity vec to angular yaw
        float flRawYawIdeal = (velocityLengthXY > 0 && entity->flags() * M_RADPI);
        if (flRawYawIdeal < 0.0f)
            flRawYawIdeal += 360.0f;

        const auto m_flMoveYawIdeal = Helpers::angleNormalize(Helpers::angleDiff(flRawYawIdeal, Animations::getFootYaw()));
    }


    if (player.layers[ANIMATION_LAYER_ADJUST].sequence == 979, entity->eyeAngles().y > 119.f)
        if (player.layers[ANIMATION_LAYER_ADJUST].sequence == 0, entity->eyeAngles().y > 0.f)
        {
            const auto m_side = 2 * int(Helpers::angleNormalize(entity->eyeAngles().y) >= 0.f) - 1;
            const auto m_resolve_value = 60.f;
        }
    /* brute */
    if (is_jitter) /* jitter brute */
    {
        switch (m_missed_shots % 3)
        {
        case ROTATION_SIDE_LEFT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y + yaw_resolve));
            break;
        case ROTATION_SIDE_CENTER:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y));
            break;
        case ROTATION_SIDE_RIGHT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y - yaw_resolve));
            break;
        }
    }
    else /* last brute */
    {
        switch (m_missed_shots % 5)
        {
        case ROTATION_SIDE_CENTER:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y));
            break;
        case ROTATION_SIDE_LEFT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y + 60.f));
            break;
        case ROTATION_SIDE_RIGHT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y - 60.f));
            break;
        case ROTATION_SIDE_LOW_LEFT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y + 30.f));
            break;
        case ROTATION_SIDE_LOW_RIGHT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y - 30.f));
            break;
        }
    }

    if (move) /* move fix */
    {
        auto footYaw = resolve_move_yaw(player, entity);
        return footYaw;

    }

    bool resolverMissed = false;
    if (!resolverMissed)
    {

        switch (m_missed_shots % 3)
        {
        case ROTATION_SIDE_CENTER:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y));
            break;
        case ROTATION_SIDE_LEFT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y + 45.f));
            break;
        case ROTATION_SIDE_RIGHT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y - 45.f));
            break;
        }


    }

    if (stand) /* stand fix */
    {


        const auto feet_delta = Helpers::angleNormalize(Helpers::angleDiff(Helpers::angleNormalize(entity->getAnimstate()->timeToAlignLowerBody), Helpers::angleNormalize((entity->eyeAngles().y))));

        const auto eye_diff = Helpers::angleDiff(entity->eyeAngles().y, (entity->getAnimstate()->eyeYaw));


        auto stopped_moving = (player.layers[ANIMATION_LAYER_ADJUST].sequence) == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING;


        auto balance_adjust = (player.layers[ANIMATION_LAYER_ADJUST].sequence) == ACT_CSGO_IDLE_TURN_BALANCEADJUST;


        bool stopped_moving_this_frame = false;
        if (entity->getAnimstate()->velocityLengthXY <= 0.1f)
        {
            auto  m_duration_moving = ROTATION_MOVE;
            auto m_duration_still = entity->getAnimstate()->lastUpdateIncrement;

            auto stopped_moving_this_frame = entity->getAnimstate()->velocityLengthXY < 0;
            m_duration_moving == 0.0f;
            m_duration_still += (entity->getAnimstate()->lastUpdateIncrement);

        }


        if (entity->getAnimstate()->velocityLengthXY == 0.0f && entity->getAnimstate()->landAnimMultiplier == 0.0f && entity->getAnimstate()->lastUpdateIncrement > 0.0f)
        {
            const auto current_feet_yaw = entity->getAnimstate()->eyeYaw;
            const auto goal_feet_yaw = entity->getAnimstate()->eyeYaw;
            auto eye_delta = current_feet_yaw - goal_feet_yaw;

            if (goal_feet_yaw < current_feet_yaw)
            {
                if (eye_delta >= 180.0f)
                    eye_delta -= 360.0f;
            }
            else if (eye_delta <= -180.0f)
                eye_delta += 360.0f;

            if (eye_delta / entity->getAnimstate()->lastUpdateIncrement > 120.f)
            {
                (player.layers[ANIMATION_LAYER_ADJUST].sequence) == 0.0f;
                (player.layers[ANIMATION_LAYER_ADJUST].sequence) == 0.0f;

            }
        }

    }
    else /* last brute */
    {
        switch (m_missed_shots % 5)
        {
        case ROTATION_SIDE_CENTER:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y));
            break;
        case ROTATION_SIDE_LEFT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y + 60.f));
            break;
        case ROTATION_SIDE_RIGHT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y - 60.f));
            break;
        case ROTATION_SIDE_LOW_LEFT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y + 30.f));
            break;
        case ROTATION_SIDE_LOW_RIGHT:
            build_server_abs_yaw(entity, Helpers::angleNormalize(entity->eyeAngles().y - 30.f));
            break;
        }
    }

}

void Resolver::setup_detect(Animations::Players& player, Entity* entity) {

    // detect if player is using maximum desync.
    if (player.layers[3].cycle == 0.f)
    {
        if (player.layers[3].weight = 0.f)
        {
            player.extended = true;
        }
    }
    /* calling detect side */
    Resolver::detect_side(entity, &player.side);
    int side = player.side;
    /* bruting vars */
    float resolve_value = 50.f;
    static float brute = 0.f;
    float fl_max_rotation = entity->getMaxDesyncAngle();
    float fl_eye_yaw = entity->getAnimstate()->eyeYaw;
    float perfect_resolve_yaw = resolve_value;
    bool fl_foword = fabsf(Helpers::angleNormalize(get_angle(entity) - get_foword_yaw(entity))) < 90.f;
    int fl_shots = player.misses;

    /* clamp angle */
    if (fl_max_rotation < resolve_value) {
        resolve_value = fl_max_rotation;
    }

    /* detect if entity is using max desync angle */
    if (player.extended) {
        resolve_value = fl_max_rotation;
    }
    /* setup brting */
    if (fl_shots == 0) {
        brute = perfect_resolve_yaw * (fl_foword ? -side : side);
    }
    else {
        switch (fl_shots % 3) {
        case 0: {
            brute = perfect_resolve_yaw * (fl_foword ? -side : side);
        } break;
        case 1: {
            brute = perfect_resolve_yaw * (fl_foword ? side : -side);
        } break;
        case 2: {
            brute = 0;
        } break;
        }
    }

    /* fix goalfeet yaw */
    entity->getAnimstate()->footYaw = fl_eye_yaw + brute;
}

Vector calc_angle(const Vector source, const Vector entity_pos) {
    const Vector delta{ source.x - entity_pos.x, source.y - entity_pos.y, source.z - entity_pos.z };
    const auto& [x, y, z] = cmd->viewangles;
    Vector angles{ Helpers::rad2deg(atan(delta.z / hypot(delta.x, delta.y))) - x, Helpers::rad2deg(atan(delta.y / delta.x)) - y, 0.f };
    if (delta.x >= 0.f)
        angles.y += 180;
    return angles;
}


void Resolver::UpdateShots(UserCmd* cmd) noexcept
{
    if (!localPlayer)
        return;

    if (!localPlayer->isAlive())
    {
        tick.clear();
        bulletImpacts.clear();
        for (auto& record : shot)
            record.clear();
        return;
    }
    return;

}

void Resolver::processMissedShots() noexcept
{
    if (!resolver)
    {
        snapshots.clear();
        return;
    }

    if (!localPlayer)
    {
        snapshots.clear();
        return;
    }

    if (snapshots.empty())
        return;

    if (snapshots.front().time == -1) //Didnt get data yet
        return;

    auto snapshot = snapshots.front();
    snapshots.pop_front(); //got the info no need for this
    const auto& time = localPlayer->isAlive() ? localPlayer->tickBase() * memory->globalVars->intervalPerTick : memory->globalVars->currenttime;
    if (fabs(time - snapshot.time) > 1.f)
    {
        if (snapshot.gotImpact)
            Logger::addLog(c_xor("missed shot due to ping"));
        else
            Logger::addLog(c_xor("missed shot due to server rejection"));
        snapshots.clear();
        return;
    }
    if (!snapshot.player.gotMatrix)
        return;

    const auto entity = interfaces->entityList->getEntity(snapshot.playerIndex);
    if (!entity)
        return;

    const Model* model = snapshot.model;
    if (!model)
        return;

    StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);
    if (!hdr)
        return;

    StudioHitboxSet* set = hdr->getHitboxSet(0);
    if (!set)
        return;

    const auto angle = AimbotFunction::calculateRelativeAngle(snapshot.eyePosition, snapshot.bulletImpact, Vector{ });
    const auto end = snapshot.bulletImpact + Vector::fromAngle(angle) * 2000.f;

    const auto matrix = snapshot.backtrackRecord <= -1 ? snapshot.player.matrix.data() : snapshot.player.backtrackRecords.at(snapshot.backtrackRecord).matrix;

    bool resolverMissed = false;
    for (int hitbox = 0; hitbox < Hitboxes::Max; hitbox++)
    {
        if (AimbotFunction::hitboxIntersection(matrix, hitbox, set, snapshot.eyePosition, end))
        {
            resolverMissed = true;
            std::string missed = std::string(skCrypt("missed shot on ")) + entity->getPlayerName() + std::string(skCrypt(" due to resolver"));
            std::string missedBT = std::string(skCrypt("missed shot on ")) + entity->getPlayerName() + std::string(skCrypt(" due to invalid backtrack tick [")) + std::to_string(snapshot.backtrackRecord) + "]";
            std::string missedPred = std::string(skCrypt("missed shot on ")) + entity->getPlayerName() + std::string(skCrypt(" due to prediction error"));
            std::string missedJitter = std::string(skCrypt("missed shot on ")) + entity->getPlayerName() + std::string(skCrypt(" due to jitter"));
            if (snapshot.backtrackRecord == 1 && config->backtrack.enabled)
               Logger::addLog(missedJitter);
            else if (snapshot.backtrackRecord > 1 && config->backtrack.enabled)
                Logger::addLog(missedBT);
            else
                Logger::addLog(missed);
            Animations::setPlayer(snapshot.playerIndex)->misses++;
            break;
        }
    }
    if (!resolverMissed)
        Logger::addLog(std::string(skCrypt("missed shot due to spread")));
}

void Resolver::runPreUpdate(Animations::Players player, Animations::Players prev_player, Entity* entity) noexcept
{
    if (!resolver)
        return;

    const auto misses = player.misses;
    if (!entity || !entity->isAlive())
        return;

    if (!entity->isDormant())
        return;

    if (player.chokedPackets <= 0)
        return;

    if (snapshots.empty())
        return;

    auto& [snapshot_player, model, eyePosition, bulletImpact, gotImpact, time, playerIndex, backtrackRecord] = snapshots.front();

    Resolver::setup_detect(player, entity);
    Resolver::resolve_entity(player, prev_player, entity);
    Resolver::initialize(entity, player_record, original_goal_feet_yaw, original_pitch);
}

void Resolver::runPostUpdate(Animations::Players player, Animations::Players prev_player, Entity* entity) noexcept
{
    if (!resolver)
        return;

    const auto misses = player.misses;
    if (!entity || !entity->isAlive())
        return;

    if (!entity->isDormant())
        return;

    if (player.chokedPackets <= 0)
        return;

    if (entity->velocity().length2D() > 3.0f) {
        Animations::setPlayer(entity->index())->absAngle.y = entity->eyeAngles().y;
        return;
    }

    if (snapshots.empty())
        return;

    auto& [snapshot_player, model, eyePosition, bulletImpact, gotImpact, time, playerIndex, backtrackRecord] = snapshots.front();

    Resolver::setup_detect(player, entity);
    Resolver::resolve_entity(player, prev_player ,entity);
    Resolver::initialize(entity, player_record, original_goal_feet_yaw, original_pitch);
}

void Resolver::updateEventListeners(bool forceRemove) noexcept
{
    class ImpactEventListener : public GameEventListener {
    public:
        void fireGameEvent(GameEvent* event) {
            getEvent(event);
        }
    };

    static ImpactEventListener listener[4];
    static bool listenerRegistered = false;

    if (resolver && !listenerRegistered) {
        interfaces->gameEventManager->addListener(&listener[0], skCrypt("bullet_impact"));
        interfaces->gameEventManager->addListener(&listener[1], skCrypt("player_hurt"));
        interfaces->gameEventManager->addListener(&listener[2], skCrypt("round_start"));
        interfaces->gameEventManager->addListener(&listener[3], skCrypt("weapon_fire"));
        listenerRegistered = true;
    }

    else if ((!resolver || forceRemove) && listenerRegistered) {
        interfaces->gameEventManager->removeListener(&listener[0]);
        interfaces->gameEventManager->removeListener(&listener[1]);
        interfaces->gameEventManager->removeListener(&listener[2]);
        interfaces->gameEventManager->removeListener(&listener[3]);
        listenerRegistered = false;
    }
}

float Resolver::resolve_pitch(Entity* entity)
{
    if (!(entity->flags() & FL_ONGROUND) && entity->eyeAngles().x >= 178.36304f)
        pitch = -89.0f;
    else
    {
        if (fabs(entity->eyeAngles().x) > 89.0f)
            pitch = entity->index() % 4 != 3 ? 89.0f : -89.0f;
        else if (entity->index() % 4 != 3)
            pitch = 89.0f;
    }

    return original_pitch;
}