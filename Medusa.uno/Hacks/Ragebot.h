#pragma once

struct UserCmd;
struct Hitboxess
{
    //"LeftUpperArm","LeftForearm","RightCalf","RightThigh","LeftCalf","LeftThigh"
    bool head{ false };
    bool upperChest{ false };
    bool chest{ false };
    bool lowerChest{ false };
    bool stomach{ false };
    bool pelvis{ false };
    bool rightUpperArm{ false };
    bool rightForeArm{ false };
    bool leftUpperArm{ false };
    bool leftForeArm{ false };
    bool rightCalf{ false };
    bool rightThigh{ false };
    bool leftCalf{ false };
    bool leftThigh{ false };
};

class optimized_adjust_data
{
public:
    int i;
    Player* player;

    float simulation_time;
    float duck_amount;

    Vector angles;
    Vector origin;

    bool shot;

    optimized_adjust_data() //-V730
    {
        reset();
    }

    void reset()
    {
        i = 0;
        player = nullptr;

        simulation_time = 0.0f;
        duck_amount = 0.0f;
        shot = false;
        angles.Zero();
        origin.Zero();
    }
};

namespace Ragebot
{
    void run(UserCmd*) noexcept;
	void updateInput() noexcept;
    bool preferBaim = false;
    struct Enemies {
        int id;
        int health;
        float distance;
        float fov;

        bool operator<(const Enemies& enemy) const noexcept {
            if (health != enemy.health)
                return health < enemy.health;
            if (fov != enemy.fov)
                return fov < enemy.fov;
            return distance < enemy.distance;
        }

        Enemies(int id, int health, float distance, float fov) noexcept : id(id), health(health), distance(distance), fov(fov) { }
    };

    struct {
        bool operator()(Enemies a, Enemies b) const
        {
            return a.health < b.health;
        }
    } healthSort;
    struct {
        bool operator()(Enemies a, Enemies b) const
        {
            return a.distance < b.distance;
        }
    } distanceSort;
    struct {
        bool operator()(Enemies a, Enemies b) const
        {
            return a.fov < b.fov;
        }
    } fovSort;

    struct scan_data
    {
        Vector aim_point = ZERO;
        int hitbox = -1;
        int damage = -1;
    };
}