#pragma once

#include "Animations.h"

#include "../SDK/GameEvent.h"
#include "../SDK/Entity.h"

#define RAD2DEG(x) DirectX::XMConvertToDegrees(x)
#define DEG2RAD(x) DirectX::XMConvertToRadians(x)

//class adjust_data;
enum resolver_side
{
	RESOLVER_ORIGINAL,
	RESOLVER_ZERO,
	RESOLVER_FIRST,
	RESOLVER_SECOND,
	RESOLVER_LOW_FIRST,
	RESOLVER_LOW_SECOND,
	RESOLVER_LOW_FIRST_20,
	RESOLVER_LOW_SECOND_20,
	RESOLVER_ON_SHOT,
	RESOLVER_LOW_POSITIVE,
	RESOLVER_LOW_NEGATIVE,
	RESOLVER_POSITIVE,
	RESOLVER_NEGATIVE,
};
enum resolver_history
{
	HISTORY_UNKNOWN = -1,
	HISTORY_ORIGINAL,
	HISTORY_ZERO,
	HISTORY_DEFAULT,
	HISTORY_LOW
};
enum resolver_type
{
	ORIGINAL,
	BRUTEFORCE,
	LBY,
	LAYERS,
	TRACE,
	DIRECTIONAL,
	ENGINE,
	FREESTAND,
	HURT,
	NON_RESOLVED
};

enum Typpes
{
	JITTER,
	STATIC
};

enum Mode
{
	AIR,
	MOVING,
	STANDING
};

enum JitterResolve
{
	JITTER1,
	JITTER2,
	NOPEE
};

enum ROTATE_MODE
{
	ROTATE_SERVER,
	ROTATE_LEFT,
	ROTATE_CENTER,
	ROTATE_RIGHT,
	ROTATE_LOW_LEFT,
	ROTATE_LOW_RIGHT
};

enum simulate_side_t
{
	side_right = -1,
	side_zero,
	side_left,
	side_original,
};

class ang_t {
public:
	// data member variables.
	float x, y, z;

public:
	// constructors.
	__forceinline ang_t() : x{}, y{}, z{} {}
	__forceinline ang_t(float x, float y, float z) : x{ x }, y{ y }, z{ z } {}

	// at-accesors.
	__forceinline float& at(const size_t index) {
		return ((float*)this)[index];
	}

	__forceinline float& at(const size_t index) const {
		return ((float*)this)[index];
	}

	// index operators.
	__forceinline float& operator( )(const size_t index) {
		return at(index);
	}

	__forceinline const float& operator( )(const size_t index) const {
		return at(index);
	}

	__forceinline float& operator[ ](const size_t index) {
		return at(index);
	}

	__forceinline const float& operator[ ](const size_t index) const {
		return at(index);
	}

	// equality operators.
	__forceinline bool operator==(const ang_t& v) const {
		return v.x == x && v.y == y && v.z == z;
	}

	__forceinline bool operator!=(const ang_t& v) const {
		return v.x != x || v.y != y || v.z != z;
	}

	__forceinline bool operator !() const {
		return !x && !y && !z;
	}

	// copy assignment.
	__forceinline ang_t& operator=(const ang_t& v) {
		x = v.x;
		y = v.y;
		z = v.z;

		return *this;
	}

	// negation-operator.
	__forceinline ang_t operator-() const {
		return ang_t(-x, -y, -z);
	}

	// arithmetic operators.
	__forceinline ang_t operator+(const ang_t& v) const {
		return {
			x + v.x,
			y + v.y,
			z + v.z
		};
	}

	__forceinline ang_t operator-(const ang_t& v) const {
		return {
			x - v.x,
			y - v.y,
			z - v.z
		};
	}

	__forceinline ang_t operator*(const ang_t& v) const {
		return {
			x * v.x,
			y * v.y,
			z * v.z
		};
	}

	__forceinline ang_t operator/(const ang_t& v) const {
		return {
			x / v.x,
			y / v.y,
			z / v.z
		};
	}

	// compound assignment operators.
	__forceinline ang_t& operator+=(const ang_t& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	__forceinline ang_t& operator-=(const ang_t& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	__forceinline ang_t& operator*=(const ang_t& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	__forceinline ang_t& operator/=(const ang_t& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	// arithmetic operators w/ float.
	__forceinline ang_t operator+(float f) const {
		return {
			x + f,
			y + f,
			z + f
		};
	}

	__forceinline ang_t operator-(float f) const {
		return {
			x - f,
			y - f,
			z - f
		};
	}

	__forceinline ang_t operator*(float f) const {
		return {
			x * f,
			y * f,
			z * f
		};
	}

	__forceinline ang_t operator/(float f) const {
		return {
			x / f,
			y / f,
			z / f
		};
	}

	// compound assignment operators w/ float.
	__forceinline ang_t& operator+=(float f) {
		x += f;
		y += f;
		z += f;
		return *this;
	}

	__forceinline ang_t& operator-=(float f) {
		x -= f;
		y -= f;
		z -= f;
		return *this;
	}

	__forceinline ang_t& operator*=(float f) {
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	__forceinline ang_t& operator/=(float f) {
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	// methods.
	__forceinline void clear() {
		x = y = z = 0.f;
	}

	__forceinline void normalize() {
		Helpers::angleNormalize(x);
		Helpers::angleNormalize(y);
		Helpers::angleNormalize(z);
	}

	__forceinline ang_t normalized() const {
		auto vec = *this;
		vec.normalize();
		return vec;
	}

//	__forceinline void clamp() {
//		Helpers::clamp(x, -89.f, 89.f);
//		Helpers::clamp(y, -180.f, 180.f);
//		Helpers::clamp(z, -90.f, 90.f);
//	}

	//__forceinline void SanitizeAngle() {
	//	Helpers::angleNormalize(y);
	//	clamp();
	//}
};

class AimPlayer {

public:

	// netvars.
	float  m_sim_time;
	Vector m_origin;
	float  m_body;
	float  m_anim_time;
};

class adjust_data //-V730
{
public:
	 std::array< AimPlayer, 64 > m_players;
		AimPlayer m_walk_record1;
		AnimationLayer layers[13];
		bool player_records[65];
		resolver_type type;
		resolver_side side;
		JitterResolve Jitter;
		Mode Modes;
		Typpes Types;
		float Right;
		float Middle;
		float Left;
		float desync_amount;
		Player* player;
		bool m_valid{ false };
		AnimationLayer resolver_layers[3][13];

		// get current desync angle
		float m_dsyangle{ 0.f };

		// side applied to angle
		int m_side{ 0 };

		// set roll angle
		bool m_roll{ false };

		// layers before animfix start (used for calculations)
		AnimationLayer m_layers[13]{};
		AnimationLayer m_layer[13]{};

		// did we find layer ?
		bool m_layer_init{ false };

		// side applied in resolver modes
		int m_curside{ 0 };
		int m_lastside{ 0 };

		// did we apply history resolver ?
		bool m_history{ false };

		// current resolver mode:
	// 1 - freestanding
	// 2 - animations
	// 3 - brute
	// 4 - brute Z
		int m_mode{ 0 };

		bool invalid;
		Vector origin;
		float simulation_time;
		bool shot;

		float duck_amount;

		Vector velocity;
		bool resolved{};

		bool   m_fake_walk;
		float  m_body;
		int       m_body_index;
		bool      m_has_body_updated;
		int       m_last_moving_index;
		int		  m_stand_index;
		int		  m_stand_index1;
		int       m_stand_index3;
		int       m_reverse_fs;
		int		  m_last_move;
		int    m_flags;
		int m_walk_record;
		Vector m_velocity;
		int       m_shots;
		Vector m_pred_origin;
		Vector           m_anim_velocity;
		int     m_lag;
		bool   m_shot;
		ang_t  m_eye_angles;
		Vector vecEyeAngles{};
		using records_t = std::deque< std::shared_ptr< adjust_data > >;
		records_t m_records;


		bool    m_dormant;
		__forceinline bool dormant() {
			return m_dormant;
		}
		Vector m_origin;
		bool      m_moved;
		float     m_best_angle;
		float  m_anim_time;

		int iResolveSide = 0;
		float flResolveDelta{};
};

enum EMatrixType : int {

	VISUAL,
	RESOLVE,
	LEFT,
	RIGHT,
	CENTER,
	MAX
};

namespace Resolver
{
	Entity* player = nullptr;
	adjust_data* player_record = nullptr;
	int ResolveSide[64];
	bool side = false;
	bool fake = false;
	bool was_first_bruteforce = false;
	bool was_second_bruteforce = false;
	float original_goal_feet_yaw = 0.0f;
	float original_pitch = 0.0f;
	int jitter_side;
	bool updating_animation;
	int ResolverSide;
	bool brutforced_first = false;
	bool brutforced_second = false;
	int missed_shots[65];
	float desynclog[65];
	int FreestandSide[65];
	float desync_angle = 0.0f;
	bool should_force_safepoint = false;
	float desync = 0.0f;
	float pitch = FLT_MAX;
	bool bDidShot{};

	// resolver stuff.
	size_t m_mode;

	void reset();

	inline adjust_data resolver_info[65]{};

	float max_desync_delta(Entity* player);

	bool DoesHaveJitter(Entity* player);

	//void resolve_entity(Animations::Players& player, Entity* entity);

	resolver_side last_side = RESOLVER_ORIGINAL;

	void MovingLayers();

	void StoreAntifreestand();

	void ResolveStand();

	void ApplyAngle(Entity* entity);

	bool DoesHaveFakeAngles(Entity* entity);

	bool GetLowDeltaState(Entity* entity);

	void ResolveAir();

	void ResolveJitter();

	float BruteForce(Entity* entity, bool roll);

	int detect_freestanding(Entity* entity);

	float GetAwayAngle(adjust_data* record);

	bool IsYawSideways(Entity* entity, float yaw);

	void ResolveAir1(adjust_data* data, Entity* entity);

	void ResolveWalk(adjust_data* data, Entity* entity);

	void SetMode(adjust_data* record);

	void MatchShot(adjust_data* data, Entity* entity);

	void SetYaw(adjust_data* pRecord, int flYaw, Entity* entity);

	void resolve_entity(Animations::Players& player, Animations::Players prev_player, Entity* entity);

	float resolve_shot(const Animations::Players& player, Entity* entity);

	void setup_detect(Animations::Players& player, Entity* entity);

	void UpdateShots(UserCmd* cmd) noexcept;

	void processMissedShots() noexcept;
	void runPreUpdate(Animations::Players player, Animations::Players prev_player, Entity* entity) noexcept;
	void runPostUpdate(Animations::Players player, Animations::Players prev_player, Entity* entity) noexcept;
	//void initialize(Entity* e, adjust_data* record, const float& goal_feet_yaw);
	void initialize(Entity* e, adjust_data* record, const float& goal_feet_yaw, const float& pitch);
	void CmdGrabber(UserCmd* cmd1);
	void saveRecord(int playerIndex, float playerSimulationTime) noexcept;

	void getEvent(GameEvent* event) noexcept;

	float GetLeftYaw(Entity* entity);

	float GetRightYaw(Entity* entity);

	//void runPreUpdate(Animations::Players player, Entity* entity) noexcept;
	//void runPostUpdate(Animations::Players player, Entity* entity) noexcept;

	void apply_side(Entity* entity, const int& choke);

	void Antionetap(int userid, Entity* entity, Vector shot);

	void detect_side(Entity* entity, int* side);

	void delta_side_detect(int* side, Entity* entity);

	void updateEventListeners(bool forceRemove = false) noexcept;

	float resolve_pitch(Entity* entity);

	struct player_settings
	{
		__int64 id;
		resolver_history res_type;
		bool low_stand;
		bool low_move;
		bool faking;
		int neg;
		int pos;

		player_settings(__int64 id, resolver_history res_type, bool low_stand, bool low_move, bool faking, int left, int right) noexcept : id(id), res_type(res_type), low_stand(low_stand), low_move(low_move), faking(faking), neg(neg), pos(pos)
		{

		}
	};
	std::vector<player_settings> player_sets;

	enum Modes : size_t {
		RESOLVE_NONE = 0,
		RESOLVE_WALK,
		RESOLVE_STAND1,//not sideways hopefully
		RESOLVE_STAND2,//sideways praying brute
		RESOLVE_STAND3,//unknown last move
		RESOLVE_REVERSEFS,
		RESOLVE_LASTMOVELBY,
		RESOLVE_BACK,
		RESOLVE_AIR,
		RESOLVE_BODY,
		RESOLVE_BODY_UPDATED,
		RESOLVE_STAND//default
	};

	struct Tick
	{
		matrix3x4 matrix[256];
		Vector origin;
		Vector mins;
		Vector max;
		float time;
	};

	struct Ticks
	{
		Vector position;
		float time;
	};

	struct Info
	{
		Info() : misses(0), hit(false) { }
		int misses;
		bool hit;
	};

	//Resolver::Ticks getClosest(const float time) noexcept;
//	extern std::array<Info, 65> player;
	extern std::deque<Ticks> bulletImpacts;
	extern std::deque<Ticks> tick;
	extern std::deque<Tick> shot[65];
	inline std::array<adjust_data, 65> m_info{ };
	inline std::array<std::vector<adjust_data>, 65> m_history{ };

	struct SnapShot
	{
		Animations::Players player;
		const Model* model{ };
		Vector eyePosition{};
		Vector bulletImpact{};
		bool gotImpact{ false };
		float time{ -1 };
		int playerIndex{ -1 };
		int backtrackRecord{ -1 };
	};

	Resolver::Ticks getClosest(const float time) noexcept;
}

class AdaptiveAngle {
public:
	float m_yaw;
	float m_dist;

public:
	// ctor.
	__forceinline AdaptiveAngle(float yaw, float penalty = 0.f) {
		// set yaw.
		m_yaw = Helpers::normalizeYaw(yaw);

		// init distance.
		m_dist = 0.f;

		// remove penalty.
		m_dist -= penalty;
	}
};

extern std::string ResolverMode[65];