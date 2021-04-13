#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#include <array>
#include <cstdint>
#include <algorithm>

#define PI 3.1459f

class Shmup : public olc::PixelGameEngine
{
public:
	Shmup()
	{
		sAppName = "Shmup Test";
	}

	olc::vf2d playerPos;
	
	olc::Sprite* characterAtlasSprite;
	olc::Sprite* bulletAtlasSprite;

	olc::Decal* characterAtlasDecal;
	olc::Decal* bulletAtlasDecal;

	float playerSpeed = 20.0f;
	float hitboxAngle = 0.0f;
	int playerHealth = 3;
	int playerScore = 0;
	bool playerCanFire = false;
	bool playerTookDamage = false;
	bool playerAlive = true;
	float playerCounter = 0;

	float worldSpeed = 5.0f;

	double worldPos = 0.0;

	bool gameWin = false;

	std::array<olc::vf2d, 500> arrStars;
	std::string playerScoreString;

	float playerReloadTimer = 0.0f;
	float playerReloadDelay = 0.2;
	float frameTimer = 0.0f;
	uint8_t frame = 0;

	struct Bullet
	{
		olc::vf2d pos;
		olc::vf2d vel;
		uint32_t id = 0;
		bool remove = false;
		float angle = 0;
	};

	struct Enemy;

	struct EnemyDefinition
	{
		double triggerTime = 0.0f;
		uint32_t id = 0;
		int health = 0;
		float offset = 0.0f;
		int score = 0;

		std::function<void(Enemy&, float, float)> enemyMove;
		std::function<void(Enemy&, float, float, std::list<Bullet>&)> enemyFire;

		bool tookDamage = false;
	};

	struct Enemy
	{
		olc::vf2d pos;
		EnemyDefinition def;

		std::array<float, 4> dataMove{ 0 };
		std::array<float, 4> dataFire{ 0 };

		void Update(float deltaTime, float scrollSpeed, std::list<Bullet>& bullets)
		{
			def.enemyMove(*this, deltaTime, scrollSpeed);
			def.enemyFire(*this, deltaTime, scrollSpeed, bullets);
		}
	};

	std::list<EnemyDefinition> spawnList;
	std::list<Enemy> enemyList;
	std::list<Bullet> bulletList;
	std::list<Bullet> playerBulletList;
	std::list<Bullet> fragList;

public:
	~Shmup()
	{
		delete characterAtlasSprite;
		delete characterAtlasDecal;
		delete bulletAtlasSprite;
		delete bulletAtlasDecal;
	}
	bool OnUserCreate() override
	{
		characterAtlasSprite = new olc::Sprite("res//gfx//character_atlas.png");
		bulletAtlasSprite = new olc::Sprite("res//gfx//bullet_atlas.png");

		characterAtlasDecal = new olc::Decal(characterAtlasSprite);
		bulletAtlasDecal = new olc::Decal(bulletAtlasSprite);

		playerPos =
		{
			(float)(ScreenWidth() >> 1), (float)(ScreenHeight() >> 1)
		};

		for (auto& s : arrStars) s =
		{
			(float)(rand() % ScreenWidth()), (float)(rand() % ScreenHeight())
		};

		//MOVEMENT PATTERNS
		auto MoveNone = [&](Enemy& e, float deltaTime, float scrollSpeed)
		{
			e.pos.y += scrollSpeed * deltaTime;
		};

		auto MoveFast = [&](Enemy& e, float deltaTime, float scrollSpeed)
		{
			e.pos.y += scrollSpeed * deltaTime * 2.0f;
		};

		auto MoveSlow = [&](Enemy& e, float deltaTime, float scrollSpeed)
		{
			e.pos.y += scrollSpeed * deltaTime * 0.5f;
		};

		auto MoveVerySlow = [&](Enemy& e, float deltaTime, float scrollSpeed)
		{
			e.pos.y += scrollSpeed * deltaTime * 0.25f;
		};

		auto MoveSinusoidNarrow = [&](Enemy& e, float deltaTime, float scrollSpeed)
		{
			e.dataMove[0] += deltaTime * 0.1;
			e.pos.y += scrollSpeed * deltaTime;
			e.pos.x += 5.0f * cosf(e.dataMove[0]) * deltaTime;
		};

		auto MoveSinusoidWide = [&](Enemy& e, float deltaTime, float scrollSpeed)
		{
			e.dataMove[0] += deltaTime * 0.1;
			e.pos.y += scrollSpeed * deltaTime;
			e.pos.x += 10.0f * cosf(e.dataMove[0]) * deltaTime;
		};

		auto MoveSinusoidWideSlow = [&](Enemy& e, float deltaTime, float scrollSpeed)
		{
			e.dataMove[0] += deltaTime * 0.1;
			e.pos.y += scrollSpeed * deltaTime * 0.5f;
			e.pos.x += 10.0f * cosf(e.dataMove[0]) * deltaTime;
		};

		auto MoveSinusoidWideVerySlow = [&](Enemy& e, float deltaTime, float scrollSpeed)
		{
			e.dataMove[0] += deltaTime * 0.1;
			e.pos.y += scrollSpeed * deltaTime * 0.25f;
			e.pos.x += 10.0f * cosf(e.dataMove[0]) * deltaTime;
		};


		//FIRING PATTERNS
		auto FireNone = [&](Enemy& e, float deltaTime, float scrollSpeed, std::list<Bullet>& bullets)
		{

		};

		auto FireStraight = [&](Enemy& e, float deltaTime, float scrollSpeed, std::list<Bullet>& bullets)
		{
			constexpr float delay = 10.0f;
			e.dataFire[0] += deltaTime;
			if (e.dataFire[0] >= delay)
			{
				e.dataFire[0] -= delay;
				Bullet b;
				b.id = e.def.id;
				b.pos = e.pos + olc::vf2d(16, 32);
				b.vel = { 0.0f, 10.0f };
				bullets.push_back(b);
			}
		};

		auto FireStraightFast = [&](Enemy& e, float deltaTime, float scrollSpeed, std::list<Bullet>& bullets)
		{
			constexpr float delay = 5.0f;
			e.dataFire[0] += deltaTime;
			if (e.dataFire[0] >= delay)
			{
				e.dataFire[0] -= delay;
				Bullet b;
				b.id = e.def.id;
				b.pos = e.pos + olc::vf2d(16, 32);
				b.vel = { 0.0f, 10.0f };
				bullets.push_back(b);
			}
		};

		/*auto FireAim = [&](Enemy& e, float deltaTime, float scrollSpeed, std::list<Bullet>& bullets)
		{
			constexpr float delay = 10.0f;
			//float angle = 1 / sqrt(e.pos.dot(playerPos) / (e.pos.mag() * playerPos.mag()));
			e.dataFire[0] += deltaTime;
			if (e.dataFire[0] >= delay)
			{
				e.dataFire[0] -= delay;
				Bullet b;
				b.pos = e.pos + olc::vf2d(16, 32);
				b.vel = { 10.0f * sinf(playerPos.x), 10.0f * cosf(playerPos.y) };
				bullets.push_back(b);
			}
		};*/

		auto FireCirclePulse = [&](Enemy& e, float deltaTime, float scrollSpeed, std::list<Bullet>& bullets)
		{
			constexpr float delay = 10.0f;
			constexpr int bulletCount = 10;
			constexpr float theta = PI * 2.0f / (float)bulletCount;
			e.dataFire[0] += deltaTime;
			if (e.dataFire[0] >= delay)
			{
				e.dataFire[0] -= delay;

				for (int i = 0; i < bulletCount; ++i)
				{
					Bullet b;
					b.id = e.def.id;
					b.pos = e.pos + olc::vf2d(16, 16);
					b.vel = { 10.0f * cosf(theta * i), 10.0f * sinf(theta * i) };
					bullets.push_back(b);
				}
			}
		};

		auto FireCirclePulseFast = [&](Enemy& e, float deltaTime, float scrollSpeed, std::list<Bullet>& bullets)
		{
			constexpr float delay = 5.0f;
			constexpr int bulletCount = 10;
			constexpr float theta = PI * 2.0f / (float)bulletCount;
			e.dataFire[0] += deltaTime;
			if (e.dataFire[0] >= delay)
			{
				e.dataFire[0] -= delay;

				for (int i = 0; i < bulletCount; ++i)
				{
					Bullet b;
					b.id = e.def.id;
					b.pos = e.pos + olc::vf2d(16, 16);
					b.vel = { 10.0f * cosf(theta * i), 10.0f * sinf(theta * i) };
					bullets.push_back(b);
				}
			}
		};

		auto FireSpiral = [&](Enemy& e, float deltaTime, float scrollSpeed, std::list<Bullet>& bullets)
		{
			constexpr float delay = 1.0f;
			constexpr int bulletCount = 10;
			constexpr float theta = PI * 2.0f / (float)bulletCount;
			e.dataFire[0] += deltaTime;
			if (e.dataFire[0] >= delay)
			{
				e.dataFire[0] -= delay;
				e.dataFire[1] += 0.1f;

				Bullet b;
				b.id = e.def.id;
				b.pos = e.pos + olc::vf2d(16, 16);
				b.vel = { 10.0f * cosf(e.dataFire[1]), 10.0f * sinf(e.dataFire[1]) };
				bullets.push_back(b);
			}
		};


		spawnList =
		{
			{60.0, 0, 3, 0.5f, 100, MoveNone, FireNone},
			{240.0, 2, 3, 0.25f, 200, MoveSinusoidNarrow, FireStraight},
			{240.0, 2, 3, 0.75f, 200, MoveSinusoidNarrow, FireStraight},
			{360.0, 2, 3, 0.2f, 200, MoveSinusoidWide, FireStraight},
			{360.0, 1, 5, 0.5f, 300, MoveSlow, FireCirclePulse},
			{360.0, 2, 3, 0.8f, 200, MoveSinusoidWide, FireStraight},
			{480.0, 3, 10, 0.25f, 500, MoveSinusoidWide, FireSpiral},
			{480.0, 3, 10, 0.75f, 500, MoveSinusoidWide, FireSpiral},
			{560.0, 4, 25, 0.5f, 500, MoveVerySlow, FireStraight},
			{1200.0, 0, 10, 0.25f, 250, MoveNone, FireStraightFast},
			{1360.0, 0, 10, 0.75f, 250, MoveNone, FireStraightFast},
			{1500.0, 3, 15, 0.3f, 500, MoveFast, FireCirclePulseFast},
			{1600.0, 3, 15, 0.7f, 500, MoveFast, FireCirclePulseFast},
			{1750.0, 3, 20, 0.5f, 500, MoveVerySlow, FireSpiral},
			{1750.0, 4, 5, 0.7f, 500, MoveSinusoidWideVerySlow, FireCirclePulseFast},
			{1750.0, 4, 5, 0.3f, 500, MoveSinusoidWideVerySlow, FireCirclePulseFast},
		};

		return true;
	}

	bool OnUserUpdate(float deltaTime) override
	{
		if (!playerAlive)
		{
			olc_Terminate();
		}

		deltaTime *= 10;
		if (worldPos > 2100 && enemyList.size() == 0)
		{
			gameWin = true;
		}

		if(!gameWin)
		{
			//INPUT
			playerSpeed = 20.0f;
			if (GetKey(olc::SHIFT).bHeld)
			{
				playerSpeed = 10.0f;
			}
			if (GetKey(olc::UP).bHeld)
			{
				playerPos.y -= playerSpeed * deltaTime;
			}
			if (GetKey(olc::DOWN).bHeld)
			{
				playerPos.y += playerSpeed * deltaTime;
			}
			if (GetKey(olc::LEFT).bHeld)
			{
				playerPos.x -= playerSpeed * deltaTime;
			}
			if (GetKey(olc::RIGHT).bHeld)
			{
				playerPos.x += playerSpeed * deltaTime;
			}

			if (playerPos.x + 4 < 0)
			{
				playerPos.x = -4;
			}
			if (playerPos.y + 2 < 0)
			{
				playerPos.y = -2;
			}
			if (playerPos.x + 28.0f > ScreenWidth())
			{
				playerPos.x = (float)ScreenWidth() - 28.0f;
			}
			if (playerPos.y + 28.0f > ScreenHeight())
			{
				playerPos.y = (float)ScreenHeight() - 28.0f;
			}

			playerReloadTimer += deltaTime * 0.1;
			if (playerReloadTimer >= playerReloadDelay)
			{
				playerCanFire = true;
				playerReloadTimer -= playerReloadDelay;
			}

			if (GetKey(olc::Z).bHeld)
			{
				if (playerCanFire)
				{
					Bullet b;
					b.pos = { playerPos.x + 16.0f, playerPos.y };
					b.vel = { 0.0f, -20.0f };
					b.id = 0;
					playerBulletList.push_back(b);
					playerCanFire = false;
				}
			}

			worldPos += (double)worldSpeed * (double)deltaTime;

			while (!spawnList.empty() && worldPos >= spawnList.front().triggerTime)
			{
				Enemy e;
				e.def = spawnList.front();
				e.pos =
				{
					e.def.offset * ((float)ScreenWidth()) - ((float)characterAtlasSprite->width * 0.125f) * 0.5f ,
					0.0f - ((float)characterAtlasSprite->height * 0.125f)
				};

				spawnList.pop_front();
				enemyList.push_back(e);
			}

			//Update Fragments
			for (auto& f : fragList)
			{
				f.pos += (f.vel + olc::vf2d(0.0f, worldSpeed)) * deltaTime;
			}

			//Update Enemies
			for (auto& e : enemyList)
			{
				e.Update(deltaTime, worldSpeed, bulletList);
			}

			//Update Enemy Bullets
			for (auto& b : bulletList)
			{
				b.pos += (b.vel + olc::vf2d(0.0f, worldSpeed)) * deltaTime;

				if ((b.pos - (playerPos + olc::vf2d(16, 16))).mag2() < 6.0f * 6.0f)
				{
					b.remove = true;
					playerHealth -= 1;
					if (playerHealth <= 0)
					{
						playerAlive = false;
					}
					playerTookDamage = true;
				}

				b.angle += deltaTime;
				if (b.angle > 360)
				{
					b.angle -= 360;
				}
			}

			//Update Player Bullets
			for (auto& b : playerBulletList)
			{
				b.pos += (b.vel + olc::vf2d(0.0f, worldSpeed)) * deltaTime * 2;

				for (auto& e : enemyList)
				{
					if ((b.pos - (e.pos + olc::vf2d(16, 16))).mag2() < 24.0f * 24.0f)
					{
						b.remove = true;
						e.def.health -= 1;
						e.def.tookDamage = true;

						if (e.def.health <= 0)
						{
							playerScore += e.def.score;
							for (int i = 0; i < 1000 + (9000 * (e.def.id == 5)); ++i)
							{
								float angle = ((float)rand() / (float)RAND_MAX) * 2.0f * PI;
								float speed = ((float)rand() / (float)RAND_MAX) * 20.0f + 25.0f;
								fragList.push_back
								({
									e.pos + olc::vf2d(16.0f, 16.0f),
									{speed * cosf(angle), speed * sinf(angle)},
									e.def.id
									});
							}
						}
						else
						{

						}
					}
				}

				b.angle += deltaTime;
				if (b.angle > 360)
				{
					b.angle -= 360;
				}
			}

			//Cleanup Enemies
			enemyList.remove_if([&](const Enemy& e)
				{
					return e.pos.y >= (float)ScreenHeight() || e.def.health <= 0;
				});

			//Cleanup Enemy Bullets
			bulletList.remove_if([&](const Bullet& b)
				{
					return b.pos.x < 0.0f || b.pos.x >(float)ScreenWidth() || b.pos.y > ScreenHeight() || b.pos.y < 0 || b.remove;
				});

			//Cleanup Player Bullets
			playerBulletList.remove_if([&](const Bullet& b)
				{
					return b.pos.x < 0.0f || b.pos.x >(float)ScreenWidth() || b.pos.y > ScreenHeight() || b.pos.y < 0 || b.remove;
				});

			//Cleanup Bullet Fragments
			fragList.remove_if([&](const Bullet& f)
				{
					return f.pos.x < 0.0f || f.pos.x >(float)ScreenWidth() || f.pos.y > ScreenHeight() || f.pos.y < 0 || f.remove;
				});

			//DISPLAY
			Clear(olc::BLACK);

			//Draw Stars
			for (size_t i = 0; i < arrStars.size(); ++i)
			{
				auto& s = arrStars[i];

				s.y += worldSpeed * deltaTime * ((i < 300) ? 0.8f : 1.0f);
				if (s.y > (float)ScreenHeight())
				{
					s =
					{
						(float)(rand() % ScreenWidth()), 0.0f
					};
				}
				Draw(s, ((i < 300) ? olc::VERY_DARK_GREY : olc::WHITE));
			}

			SetPixelMode(olc::Pixel::MASK);

			//Draw Player
			constexpr float delay = 1.0f;
			playerCounter += deltaTime;
			if (playerCounter >= delay)
			{
				playerCounter -= delay;
				playerTookDamage = false;
			}

			DrawPartialDecal(playerPos + olc::vf2d(0, 32), olc::vf2d(32.0f, -32.0f), characterAtlasDecal, olc::vf2d(0, 16 * playerTookDamage), olc::vf2d(16, 16));


			//Draw Enemies
			constexpr float frameDelay = 1.5f;
			frameTimer += deltaTime;
			if (frameTimer >= frameDelay)
			{
				frameTimer -= frameDelay;
				frame++;
				if (frame >= 2)
				{
					frame = 0;
				}
			}
			for (auto& e : enemyList)
			{
				switch (e.def.id)
				{
				case 0:
					DrawPartialDecal(e.pos, olc::vf2d(32.0f, 32.0f), characterAtlasDecal, olc::vf2d(16 * e.def.id, 16 * e.def.tookDamage), olc::vf2d(16, 16));
					break;
				case 1:
					DrawPartialDecal(e.pos, olc::vf2d(32.0f, 32.0f), characterAtlasDecal, olc::vf2d(16 * e.def.id, 16 * e.def.tookDamage), olc::vf2d(16, 16));
					break;
				case 2:
					DrawPartialDecal(e.pos, olc::vf2d(32.0f, 32.0f), characterAtlasDecal, olc::vf2d(16 * (e.def.id + frame), 16 * e.def.tookDamage), olc::vf2d(16, 16));
					break;
				case 3:
					DrawPartialDecal(e.pos, olc::vf2d(32.0f, 32.0f), characterAtlasDecal, olc::vf2d(16 * (e.def.id + 2), 16 * e.def.tookDamage), olc::vf2d(16, 16));
					break;
				case 4:
					DrawPartialDecal(e.pos, olc::vf2d(32.0f, 32.0f), characterAtlasDecal, olc::vf2d(16 * (e.def.id + 2), 16 * e.def.tookDamage), olc::vf2d(16, 16));
					break;
				case 5:
					DrawPartialDecal(e.pos, olc::vf2d(32.0f, 32.0f), characterAtlasDecal, olc::vf2d(16 * (e.def.id + 2), 16 * e.def.tookDamage), olc::vf2d(16, 16));
					break;
				}

				constexpr float delay = 1.5f;
				e.dataMove[3] += deltaTime;
				if (e.dataMove[3] >= delay)
				{
					e.dataMove[3] -= delay;
					e.def.tookDamage = false;
				}
			}

			//Draw Enemy Bullets
			for (auto& b : bulletList)
			{
				DrawPartialRotatedDecal(b.pos, bulletAtlasDecal, b.angle, olc::vf2d(3, 3), olc::vf2d(6 * b.id, 0), olc::vf2d(6, 6), olc::vf2d(2, 2));
			}

			//Draw Player Bullets
			for (auto& b : playerBulletList)
			{
				DrawPartialRotatedDecal(b.pos, bulletAtlasDecal, b.angle, olc::vf2d(3, 3), olc::vf2d(6 * b.id, 0), olc::vf2d(6, 6), olc::vf2d(2, 2));
			}

			//Draw Enemy Frags
			for (auto& f : fragList)
			{
				switch (f.id)
				{
				case 0:
					Draw(f.pos, olc::Pixel(40, 190, 40));
					break;
				case 1:
					Draw(f.pos, olc::Pixel(40, 182, 190, 255));
					break;
				case 2:
					Draw(f.pos, olc::Pixel(190, 40, 40, 255));
					break;
				case 3:
					Draw(f.pos, olc::Pixel(151, 40, 190));
					break;
				case 4:
					Draw(f.pos, olc::Pixel(40, 190, 40));
					break;
				case 5:
					Draw(f.pos, olc::Pixel(151, 40, 190));
					break;
				}
			}

			//Draw Hitbox
			if (GetKey(olc::SHIFT).bHeld)
			{
				DrawPartialRotatedDecal(playerPos + olc::vf2d(16, 16), bulletAtlasDecal, hitboxAngle, olc::vf2d(3, 3), olc::vf2d(0, 0), olc::vf2d(6, 6), olc::vf2d(2, 2));
			}
			hitboxAngle += deltaTime;
			if (hitboxAngle > 360)
			{
				hitboxAngle -= 360;
			}

			SetPixelMode(olc::Pixel::NORMAL);

			//Draw HUD
			DrawString(4, 4, "HEALTH:");
			for (int i = 0; i < playerHealth; ++i)
			{
				FillRect(60 + (i * 10), 4, 8, 8, playerTookDamage ? olc::RED : olc::GREEN);
			}
			playerScoreString = "SCORE:" + std::to_string(playerScore);
			DrawString(4, 12, playerScoreString);
		}
		else
		{
			Clear(olc::BLACK);
			DrawString(ScreenWidth() / 2, ScreenHeight() / 2, "YOU WIN!");
			for (size_t i = 0; i < arrStars.size(); ++i)
			{
				auto& s = arrStars[i];

				s.y += worldSpeed * deltaTime * ((i < 300) ? 0.8f : 1.0f);
				if (s.y > (float)ScreenHeight())
				{
					s =
					{
						(float)(rand() % ScreenWidth()), 0.0f
					};
				}
				Draw(s, ((i < 300) ? olc::VERY_DARK_GREY : olc::WHITE));
			}
			for (auto& f : fragList)
			{
				f.pos += (f.vel + olc::vf2d(0.0f, worldSpeed)) * deltaTime;
				switch (f.id)
				{
				case 0:
					Draw(f.pos, olc::Pixel(40, 190, 40));
					break;
				case 1:
					Draw(f.pos, olc::Pixel(40, 182, 190, 255));
					break;
				case 2:
					Draw(f.pos, olc::Pixel(190, 40, 40, 255));
					break;
				case 3:
					Draw(f.pos, olc::Pixel(151, 40, 190));
					break;
				case 4:
					Draw(f.pos, olc::Pixel(40, 190, 40));
					break;
				case 5:
					Draw(f.pos, olc::Pixel(151, 40, 190));
					break;
				}
			}
		}

		return true;
	}
};


int main()
{
	Shmup demo;
	if (demo.Construct(640, 480, 2, 2))
		demo.Start();

	return 0;
}