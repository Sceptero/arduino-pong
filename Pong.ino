#define ILI9341_CS_PIN 10
#define ILI9341_DC_PIN 9
#define ILI9341_SAVE_SPCR 0

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 316

#define P1_COLOR ILI9341_CYAN
#define P2_COLOR ILI9341_GREEN

#include <SPI.h>
#include <PDQ_GFX.h>
#include <PDQ_FastPin.h>
#include <PDQ_ILI9341.h>
#include <gfxfont.h>
#include <math.h>

enum Moves
{
	MOVE_LEFT = -1,
	MOVE_RIGHT = 1,
};

enum GameStates
{
	START_SCREEN,
	MATCH,
	P1_GOAL,
	P2_GOAL,
	P1_WIN,
	P2_WIN,
};

enum Players
{
	PLAYER_1 = 1,
	PLAYER_2 = 2,
};

class Ball
{
public:
	short x, y;
	short old_x, old_y;
	short dir[2];
	unsigned long cooldown;

	Ball(short screen_width, short screen_height)
	{
		x = ceil(screen_width / 2);
		y = ceil(screen_height / 2);
		dir[0] = 1 + (random(2)*-2);
		dir[1] = 1 + (random(2)*-2);
		cooldown = 0;
		old_x = x;
		old_y = y;
	};

	void reset(short screen_width, short screen_height)
	{
		dir[0] = 1 + (random(2)*-2);
		dir[1] = 1 + (random(2)*-2);
		cooldown = 0;
		x = ceil(screen_width / 2);
		y = ceil(screen_height / 2);
	}
};

class Player
{
public:
	short x, y, size;
	short old_x;
	short score;
	unsigned long cooldown;

	Player(short y, short size, short screen_width) : y(y), size(size)
	{
		x = ceil(screen_width / 2) - ceil(size / 2) - 1;
		score = 0;
		cooldown = 0;
		old_x = x;
	};

	void reset(short screen_width)
	{
		x = ceil(screen_width / 2) - ceil(size / 2) - 1;
		old_x = x;
		cooldown = 0;
	}
};

class Pong
{
public:
	short screen_width, screen_height;
	short ball_speed;
	const short ball_base_speed = 19, player_speed = 13;
	short game_state;
	Ball *ball;
	Player *p1, *p2;

	Pong(short screen_width, short screen_height) : screen_width(screen_width), screen_height(screen_height)
	{
		ball_speed = ball_base_speed;
		game_state = GameStates::START_SCREEN;

		ball = new Ball(screen_width, screen_height);
		p1 = new Player(15, 50, screen_width);
		p2 = new Player(screen_height - 15, 50, screen_width);
	};

	void movePlayer(Player *p, short direction)
	{
		short new_pos = p->x + direction * 3;
		if (new_pos < 0) new_pos = 0;
		else if (new_pos > screen_width - p->size) new_pos = screen_width - p->size;

		p->x = new_pos;
	};

	void moveBall()
	{
		// bounce of walls
		if (ball->x + ball->dir[0] + 3 >= screen_width || ball->x + ball->dir[0] < 3) ball->dir[0] *= -1;

		// bounce of players / score goal
		if (ball->y + ball->dir[1] + 3 >= p2->y) // P2 border
		{
			if (ball->x + ball->dir[0] + 3 < p2->x || ball->x + ball->dir[0] - 3 >= p2->x + p2->size) // p1 scores a goal
			{
				p1->score++;
				if (p1->score >= 3) game_state = GameStates::P1_WIN;
				else game_state = GameStates::P1_GOAL;
				ball->dir[0] = 0;
				ball->dir[1] = 0;
			}
			else // bounce of player
			{
				short tmp = floor(p2->size / 3); // size of the part of paddle which changes x dir
				if ((ball->x < p2->x + tmp) && ball->dir[0] > -3) ball->dir[0] -= 1; // bounce left
				else if ((ball->x >= p2->x + p2->size - tmp) && ball->dir[0] < 3) ball->dir[0] += 1; // bounce right
				ball->dir[1] *= -1; // change y dir
				if (ball_speed > 4) ball_speed -= 3; // make ball faster
			}
		}
		else if (ball->y + ball->dir[1] - 5 <= p1->y) // P1 border
		{
			if (ball->x + ball->dir[0] + 3 < p1->x || ball->x + ball->dir[0] - 3 >= p1->x + p1->size) // p2 scores a goal
			{
				p2->score++;
				if (p2->score >= 3) game_state = GameStates::P2_WIN;
				else game_state = GameStates::P2_GOAL;
				ball->dir[0] = 0;
				ball->dir[1] = 0;
			}
			else // bounce of player
			{
				short tmp = floor(p1->size / 3); // size of the part of paddle which changes x dir
				if ((ball->x < p1->x + tmp) && ball->dir[0] > -3) ball->dir[0] -= 1; // bounce left
				else if ((ball->x >= p1->x + p1->size - tmp) && ball->dir[0] < 3) ball->dir[0] += 1; // bounce right
				ball->dir[1] *= -1; // change y dir
				if (ball_speed > 4) ball_speed -= 3; // make ball faster
			}
		}

		ball->x += ball->dir[0];
		ball->y += ball->dir[1];
	};

	void newRound()
	{
		p1->reset(screen_width);
		p2->reset(screen_width);
		ball_speed = ball_base_speed;
		ball->reset(screen_width, screen_height);
		game_state = GameStates::MATCH;
	}

	void newMatch()
	{
		p1->score = 0;
		p2->score = 0;
		newRound();
	}
};

class View
{
public:
	PDQ_ILI9341 tft;
	Pong *pong;
	
	View(Pong *pong)
	{
		this->pong = pong;
		tft.begin();
		welcomeScreen();
	}

	void welcomeScreen()
	{
		tft.fillScreen(ILI9341_WHITE);

		// bg
		for (short i = 0; i < 150; i += 5)
		{
			tft.drawLine(120, 0, 0, i, ILI9341_RED);
			tft.drawLine(120, 0, 240, i, ILI9341_RED);
			tft.drawLine(120, 320, 0, 320-i, ILI9341_RED);
			tft.drawLine(120, 320, 240, 320-i, ILI9341_RED);
		}

		// rocket
		for (short i = 0; i < 15; i++)
			tft.drawLine(138-i, 108+i, 189-i, 140+i, ILI9341_BLACK);
		tft.fillCircle(95, 90, 50, ILI9341_BLACK);
		tft.fillCircle(95, 90, 48, ILI9341_RED);

		// text
		tft.setTextColor(ILI9341_BLACK);
		tft.setCursor(40, 160);
		tft.setTextSize(3);
		tft.print("PONG");
		tft.setCursor(40, 190);
		tft.setTextSize(1);
		tft.print("press any button to start");
	}

	void renderBoard()
	{
		// clear screen
		tft.fillScreen(ILI9341_BLACK);

		Player *p1 = pong->p1, *p2 = pong->p2;
		Ball *ball = pong->ball;
		// draw ball
		tft.fillCircle(ball->x, ball->y, 3, ILI9341_WHITE);
		// draw players
		tft.drawRect(p1->x, p1->y, p1->size, 3, P1_COLOR);
		tft.drawRect(p2->x, p2->y, p2->size, 3, P2_COLOR);
	}

	void viewScore()
	{
		tft.fillScreen(ILI9341_BLACK);
		tft.setTextSize(5);
		tft.setCursor(37, 130);
		tft.setTextColor(ILI9341_WHITE);
		tft.print("SCORE");
		tft.setTextColor(P2_COLOR);
		tft.setCursor(48, 180);
		tft.print(pong->p2->score);
		tft.setTextColor(P1_COLOR);
		tft.setCursor(150, 180);
		tft.print(pong->p1->score);
	}

	void viewGoal(Players player)
	{
		color_t player_color = P2_COLOR;
		char *msg = "GREEN";
		
		if (player == Players::PLAYER_1)
		{
			player_color = P1_COLOR;
			msg = "BLUE";
		}


		tft.fillScreen(player_color);

		for (short i = 0; i < 240; i += 4)
		{
			tft.drawLine(120, 160, i, 0, ILI9341_WHITE);
			tft.drawLine(120, 160, 240 - i, 320, ILI9341_WHITE);
		}

		for (short i = 0; i < 320; i += 4)
		{
			tft.drawLine(120, 160, 0, i, ILI9341_WHITE);
			tft.drawLine(120, 160, 240, 320 - i, ILI9341_WHITE);
		}

		tft.setTextColor(ILI9341_BLACK);
		tft.setTextSize(4);
		tft.setCursor(48, 130);
		tft.print(msg);
		tft.setCursor(50, 170);
		tft.print("SCORES!");
	}

	void viewWin(Players player)
	{
		color_t player_color = P2_COLOR;
		char *msg = "GREEN";

		if (player == Players::PLAYER_1)
		{
			player_color = P1_COLOR;
			msg = "BLUE";
		}


		tft.fillScreen(player_color);

		for (short i = 0; i < 240; i += 4)
		{
			tft.drawLine(120, 160, i, 0, ILI9341_WHITE);
			tft.drawLine(120, 160, 240 - i, 320, ILI9341_WHITE);
		}

		for (short i = 0; i < 320; i += 4)
		{
			tft.drawLine(120, 160, 0, i, ILI9341_WHITE);
			tft.drawLine(120, 160, 240, 320 - i, ILI9341_WHITE);
		}

		tft.setTextColor(ILI9341_BLACK);
		tft.setTextSize(4);
		tft.setCursor(55, 130);
		tft.print(msg);
		tft.setCursor(60, 170);
		tft.print("WINS!");
		tft.setCursor(15, 250);
		tft.setTextSize(1);
		tft.print("Press any button to start a new game");
	}

	void updateState()
	{
		Ball *ball = pong->ball;
		Player *p1 = pong->p1, *p2 = pong->p2;

		switch (pong->game_state)
		{
		case GameStates::MATCH:
			// update ball
			if (ball->x != ball->old_x || ball->y != ball->old_y)
			{
				tft.fillCircle(ball->old_x, ball->old_y, 3, ILI9341_BLACK);
				tft.fillCircle(ball->x, ball->y, 3, ILI9341_WHITE);
				ball->old_x = ball->x;
				ball->old_y = ball->y;
			}
			// update p1
			if (p1->x != p1->old_x)
			{
				tft.drawRect(p1->old_x, p1->y, p1->size, 3, ILI9341_BLACK);
				tft.drawRect(p1->x, p1->y, p1->size, 3, P1_COLOR);
				p1->old_x = p1->x;
			}
			// update p2
			if (p2->x != p2->old_x)
			{
				tft.drawRect(p2->old_x, p2->y, p2->size, 3, ILI9341_BLACK);
				tft.drawRect(p2->x, p2->y, p2->size, 3, P2_COLOR);
				p2->old_x = p2->x;
			}
			break;
		case GameStates::P1_GOAL:
			viewGoal(Players::PLAYER_1);
			delay(2000);
			viewScore();
			delay(1500);
			pong->newRound();
			renderBoard();
			break;
		case GameStates::P2_GOAL:
			viewGoal(Players::PLAYER_2);
			delay(2000);
			viewScore();
			delay(1500);
			pong->newRound();
			renderBoard();
			break;
		case GameStates::P1_WIN:
			viewGoal(Players::PLAYER_1);
			delay(2000);
			viewScore();
			delay(1500);
			viewWin(Players::PLAYER_1);
			while (digitalRead(2) == HIGH && digitalRead(3) == HIGH && digitalRead(4) == HIGH && digitalRead(5))
			{
				delay(1);
			}
			pong->newMatch();
			renderBoard();
			break;
		case GameStates::P2_WIN:
			viewGoal(Players::PLAYER_2);
			delay(2000);
			viewScore();
			delay(1500);
			viewWin(Players::PLAYER_2);
			while (digitalRead(2) == HIGH && digitalRead(3) == HIGH && digitalRead(4) == HIGH && digitalRead(5))
			{
				delay(1);
			}
			pong->newMatch();
			renderBoard();
			break;
		default:
			break;
		}

		// update ball
		if (ball->x != ball->old_x || ball->y != ball->old_y)
		{
			tft.fillCircle(ball->old_x, ball->old_y, 3, ILI9341_BLACK);
			tft.fillCircle(ball->x, ball->y, 3, ILI9341_WHITE);
			ball->old_x = ball->x;
			ball->old_y = ball->y;
		}

		// update p1
		if (p1->x != p1->old_x)
		{
			tft.drawRect(p1->old_x, p1->y, p1->size, 3, ILI9341_BLACK);
			tft.drawRect(p1->x, p1->y, p1->size, 3, ILI9341_WHITE);
			p1->old_x = p1->x;
		}

		// update p2
		if (p2->x != p2->old_x)
		{
			tft.drawRect(p2->old_x, p2->y, p2->size, 3, ILI9341_BLACK);
			tft.drawRect(p2->x, p2->y, p2->size, 3, ILI9341_WHITE);
			p2->old_x = p2->x;
		}
	}
};

///////////////////////////////////////////////
/////////// MAIN
///////////////////////////////////////////////

View *view;
Pong *pong;

void setup() {
	//Serial.begin(9600);
	randomSeed(analogRead(0));
	
	// P1 Controls
	pinMode(4, INPUT_PULLUP);
	pinMode(5, INPUT_PULLUP);

	// P2 Controls
	pinMode(2, INPUT_PULLUP);
	pinMode(3, INPUT_PULLUP);
	
	// Init Game
	pong = new Pong(SCREEN_WIDTH, SCREEN_HEIGHT);
	view = new View(pong);
	
	pong->ball->cooldown = 0;

	while (digitalRead(2) == HIGH && digitalRead(3) == HIGH && digitalRead(4) == HIGH && digitalRead(5))
	{
		delay(1);
	}

	pong->game_state = GameStates::MATCH;
	view->renderBoard();
}

void loop() {
	// Players' movement controllers
	if (digitalRead(2) == LOW && millis() >= pong->p2->cooldown)
	{
		pong->movePlayer(pong->p2, MOVE_LEFT);
		pong->p2->cooldown = millis() + pong->player_speed;
	}
	else if (digitalRead(3) == LOW && millis() >= pong->p2->cooldown)
	{
		pong->movePlayer(pong->p2, MOVE_RIGHT);
		pong->p2->cooldown = millis() + pong->player_speed;
	}

	if (digitalRead(4) == LOW && millis() >= pong->p1->cooldown)
	{
		pong->movePlayer(pong->p1, MOVE_LEFT);
		pong->p1->cooldown = millis() + pong->player_speed;
	}
	else if (digitalRead(5) == LOW && millis() >= pong->p1->cooldown)
	{
		pong->movePlayer(pong->p1, MOVE_RIGHT);
		pong->p1->cooldown = millis() + pong->player_speed;
	}

	// BALL MOVE
	if (millis() >= pong->ball->cooldown)
	{
		pong->moveBall();
		pong->ball->cooldown = millis() + pong->ball_speed;
	}

	view->updateState();
}