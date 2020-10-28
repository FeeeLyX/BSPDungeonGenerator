#include <iostream>
#include "Windows.h"
#include <ctime>
#include <vector>
#include <chrono>
#include <algorithm>
#include <GL/glut.h>
#include <stdlib.h>
//#include "audiere.h"

using namespace std;
//using namespace audiere;

static const int minsize = 6;
static const int maxmargin = 2;
static const int maxratio = 2;
static const int hallwidth = 1;

static int tilesize = 10;
static vector <vector<bool>> tiles;
static int mapwidth = 50;
static int mapheight = 50;

int nScreenWidth = 1920; // Ширина консольного окна
int nScreenHeight = 1200; // Высота консольного окна

float fPlayerX = 1.0f; // Координата игрока по оси X
float fPlayerY = 1.0f; // Координата игрока по оси Y
float fPlayerA = 0.0f; // Направление игрока
float fPlayerSize = 0.1f;

float vx = 0.0f;
float vz = -1.0f;

float fFOV = 3.14159 * 70 / 180; // Угол обзора (поле видимости)
float fDepth = 30.0f;     // Максимальная дистанция обзора

chrono::duration<float> elapsedTime;
float fElapsedTime;
auto tp1 = chrono::system_clock::now(); // Переменные для подсчета
auto tp2 = chrono::system_clock::now(); // пройденного времени

//AudioDevicePtr device = OpenDevice();
//OutputStreamPtr sound = OpenSound(device, "sound.wav", false);

class Room
{
	public:
		int x, y, h, w;
		Room()
		{
			x = 0;
			y = 0;
			w = 0;
			h = 0;
		}

		Room(int X, int Y, int W, int H)
		{
			this->x = X;
			this->y = Y;
			this->w = W;
			this->h = H;
		}

		void Draw()
		{
			for (int i = x; i < x + w; i++)
				for (int j = y; j < y + h; j++)
					tiles[i][j] = false;
		}
};

class Hall : public Room
{
	public:
		Hall(int X, int Y, int W, int H)
		{
			this->x = X;
			this->y = Y;
			this->w = W;
			this->h = H;
		}
};

class Leaf
{
	private:
		int x, y, h, w;
	public:
		Leaf* left;
		Leaf* right;
		Room* room;
		vector<Hall*> halls;

		Leaf(int X, int Y, int W, int H)
		{
			this->x = X;
			this->y = Y;
			this->h = H;
			this->w = W;
			left = NULL;
			right = NULL;
			room = NULL;

			if (Split())
			{
				if ((this->left != NULL) && (this->right != NULL))
				{
					createHall(this->left->getRoom(), this->right->getRoom());
				}
			}
			else
			{
				int leftmargin = rand() % maxmargin + 1;
				int rightmargin = rand() % maxmargin + 1;
				int topmargin = rand() % maxmargin + 1;
				int bottommargin = rand() % maxmargin + 1;
				int x = this->x + leftmargin;
				int y = this->y + topmargin;
				int w = this->w - leftmargin - rightmargin;
				int h = this->h - topmargin - bottommargin;
				room = new Room(x, y, w, h);
			}
		}

		void drawHalls()
		{
			if ((this->left == NULL) && (this->right == NULL))
				for (int i = 0; i < halls.size(); i++)
					halls[i]->Draw();
			else
			{
				this->left->drawHalls();
				this->right->drawHalls();
			}
		}

		void drawRoom()
		{
			if (this->room != NULL)
				this->room->Draw();
			else
			{
				this->left->drawRoom();
				this->right->drawRoom();
			}
		}

		void createHall(Room* l, Room* r)
		{
			int x1 = rand() % l->w + l->x;
			int x2 = rand() % r->w + r->x;
			int y1 = rand() % l->h + l->y;
			int y2 = rand() % r->h + r->y;
			w = x2 - x1;
			h = y2 - y1;
			if (w < 0)
			{
				if (h < 0)
				{
					if (rand() % 2 == 0)
					{
						halls.push_back(new Hall(x2, y1, abs(w), hallwidth));
						halls.push_back(new Hall(x2, y2, hallwidth, abs(h) + hallwidth));
					}
					else
					{
						halls.push_back(new Hall(x2, y2, abs(w) + hallwidth, hallwidth));
						halls.push_back(new Hall(x1, y2, hallwidth, abs(h)));
					}
				}
				else if (h > 0)
				{
					if (rand() % 2 == 0)
					{
						halls.push_back(new Hall(x2, y1, abs(w), hallwidth));
						halls.push_back(new Hall(x2, y1, hallwidth, abs(h) + hallwidth));
					}
					else
					{
						halls.push_back(new Hall(x2, y2, abs(w) + hallwidth, hallwidth));
						halls.push_back(new Hall(x1, y1, hallwidth, abs(h)));
					}
				}
				else // if (h == 0)
				{
					halls.push_back(new Hall(x2, y2, abs(w), hallwidth));
				}
			}
			else if (w > 0)
			{
				if (h < 0)
				{
					if (rand() % 2 == 0)
					{
						halls.push_back(new Hall(x1, y2, abs(w), hallwidth));
						halls.push_back(new Hall(x1, y2, hallwidth, abs(h)));
					}
					else
					{
						halls.push_back(new Hall(x1, y1, abs(w) + hallwidth, hallwidth));
						halls.push_back(new Hall(x2, y2, hallwidth, abs(h) + hallwidth));
					}
				}
				else if (h > 0)
				{
					if (rand() % 2 == 0)
					{
						halls.push_back(new Hall(x1, y1, abs(w) + hallwidth, hallwidth));
						halls.push_back(new Hall(x2, y1, hallwidth, abs(h)));
					}
					else
					{
						halls.push_back(new Hall(x1, y2, abs(w), hallwidth));
						halls.push_back(new Hall(x1, y1, hallwidth, abs(h) + hallwidth));
					}
				}
				else // if (h == 0)
				{
					halls.push_back(new Hall(x1, y1, abs(w), hallwidth));
				}
			}
			else // if (w == 0)
			{
				if (h < 0)
				{
					halls.push_back(new Hall(x2, y2, hallwidth, abs(h)));
				}
				else if (h > 0)
				{
					halls.push_back(new Hall(x1, y1, hallwidth, abs(h)));
				}
			}

			for (int i = 0; i < halls.size(); i++)
			{
				halls[i]->Draw();
			}
		}

		Room* getRoom()
		{
			if (this->room != NULL)
			{
				return this->room;
			}
			else
			{
				Room* leftroom = NULL;
				Room* rightroom = NULL;
				if (this->left != NULL)
				{
					leftroom = this->left->getRoom();
				}
				if (this->right != NULL)
				{
					rightroom = this->right->getRoom();
				}
				if (leftroom == NULL && rightroom == NULL)
					return NULL;
				else if (rightroom == NULL)
					return leftroom;
				else if (leftroom == NULL)
					return rightroom;
				else if (rand() % 2 == 0)
					return leftroom;
				else
					return rightroom;
			}
		}

		bool Split()
		{
			int s = rand() % 11;
			if (s < 5)
			{
				if ((this->w > 2 * minsize) && ((2 * this->h) / this->w <= maxratio))
				{
					Horizontal();
					return true;
				}
				else if ((this->h > 2 * minsize) && ((2 * this->w) / this->h <= maxratio))
				{
					Vertical();
					return true;
				}
				else return false;
			}
			else if (s < 10)
			{
				if ((this->h > 2 * minsize) && ((2 * this->w) / this->h <= maxratio))
				{
					Vertical();
					return true;
				}
				else if ((this->w > 2 * minsize) && ((2 * this->h) / this->w <= maxratio))
				{
					Horizontal();
					return true;
				}
				else return false;
			}
			else return false;
		}

	private:
		void Horizontal()
		{
			int w1 = rand() % (this->w - 2 * minsize) + minsize;
			int w2 = this->w - w1 - 1;
			int x2 = this->x + w1 + 1;
			this->left = new Leaf(this->x, this->y, w1, this->h);
			this->right = new Leaf(x2, this->y, w2, this->h);
		}

		void Vertical()
		{
			int h1 = rand() % (this->h - 2 * minsize) + minsize;
			int h2 = this->h - h1 - 1;
			int y2 = this->y + h1 + 1;
			this->left = new Leaf(this->x, this->y, this->w, h1);
			this->right = new Leaf(this->x, y2, this->w, h2);
		}
};

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, w, 0, h);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void action(unsigned char key, int x, int y)
{
	/*
	tp2 = chrono::system_clock::now();
	elapsedTime = tp2 - tp1;
	fElapsedTime = elapsedTime.count();
	tp1 = tp2;
	*/

	switch (key)
	{
		case 'a':
			fPlayerA -= (1.5f) * 0.05;
			break;
		case 'd':
			fPlayerA += (1.5f) * 0.05;
			break;
		case 'w':
			//sound->play();
			fPlayerX += sinf(fPlayerA) * 5.0f * 0.05;
			fPlayerY += cosf(fPlayerA) * 5.0f * 0.05;
			if (tiles[(int)fPlayerX][(int)fPlayerY]) { // Если столкнулись со стеной, то откатываем шаг
				fPlayerX -= sinf(fPlayerA) * 5.0f * 0.05;
				fPlayerY -= cosf(fPlayerA) * 5.0f * 0.05;
			}
			break;
		case 's':
			//sound->play();
			fPlayerX -= sinf(fPlayerA) * 5.0f * 0.05;
			fPlayerY -= cosf(fPlayerA) * 5.0f * 0.05;
			if (tiles[(int)fPlayerX][(int)fPlayerY]) { // Если столкнулись со стеной, то откатываем шаг
				fPlayerX += sinf(fPlayerA) * 5.0f * 0.05;
				fPlayerY += cosf(fPlayerA) * 5.0f * 0.05;
			}
			break;
	}
	/*
	COORD position;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	position.X = 0;
	position.Y = 0;
	SetConsoleCursorPosition(hConsole, position);
	cout << fPlayerX << ' ' << fPlayerY << ' ' << fPlayerA << ' ' << 1.0f / fElapsedTime << "                                              ";
	*/
}

/* 
void mouseMove(int x, int y)
{
	int deltax = x - nScreenWidth / 2;
	fPlayerA += deltax * 0.01f;
	SetCursorPos(nScreenWidth / 2, nScreenHeight / 2);
}
*/

//void mouseButton(int button, int state, int x, int y) {}

void display()
{
	tp2 = chrono::system_clock::now();
	elapsedTime = tp2 - tp1;
	fElapsedTime = elapsedTime.count();
	tp1 = tp2;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	/* it's free real 3D*/
	/*
	glLoadIdentity();

	// camera
	vx = sinf(fPlayerA);
	vz = -cosf(fPlayerA);

	gluLookAt(fPlayerX, 0.5f, fPlayerY, fPlayerX + vx, 0.5f, fPlayerY + vz, 0.0f, 0.0f, 0.0f);

	// ground
	glBegin(GL_QUADS);
	glColor3f(0.75f, 0.75f, 0.75f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, mapheight);
	glVertex3f(mapwidth, 0.0f, mapheight);
	glVertex3f(mapwidth, 0.0f, 0.0f);
	glEnd();

	// walls
	glColor3f(1.0f, 1.0f, 1.0f);
	for (int i = 0; i < mapwidth; i++)
	{
		for (int j = 0; j < mapheight; j++)
		{
			if (tiles[i][j])
			{
				glPushMatrix();
				glTranslatef(i + 0.5f, 0.5f, j + 0.5f);
				glutSolidCube(1.0);
				glPopMatrix();
			}
		}
	}
	
	glBegin(GL_QUADS);
	glColor3f(0.0, 0.0, 0.0);
	glVertex2f(0.0, nScreenHeight / 2.0);
	glVertex2f(nScreenWidth, nScreenHeight / 2.0);
	glColor3f(1.0, 1.0, 0.0);
	glVertex2f(nScreenWidth, 0.0);
	glVertex2f(0.0, 0.0);
	glEnd;
	*/

	glBegin(GL_LINES);
	for (int x = 0; x < nScreenWidth; x++)  // Проходим по всем X
	{
		float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV; // Направление луча

		float fDistanceToWall = 0.0f; // Расстояние до препятствия в направлении fRayAngle
		bool bHitWall = false; // Достигнул ли луч стенку
		bool bBoundary = false;

		float fEyeX = sinf(fRayAngle); // Координаты единичного вектора fRayAngle
		float fEyeY = cosf(fRayAngle);

		float step = 0.25f;
		while (!bHitWall && fDistanceToWall < fDepth) // Пока не столкнулись со стеной
		{                                                  // Или не вышли за радиус видимости
			fDistanceToWall += step;

			int nTestX = (int)(fPlayerX + fEyeX*fDistanceToWall); // Точка на игровом поле
			int nTestY = (int)(fPlayerY + fEyeY*fDistanceToWall); // в которую попал луч

			if (nTestX < 0 || nTestX >= mapwidth || nTestY < 0 || nTestY >= mapheight)
			{ // Если мы вышли за карту, то дальше смотреть нет смысла - фиксируем соударение на расстоянии видимости
				bHitWall = true;
				fDistanceToWall = fDepth;
			}
			else if (tiles[nTestX][nTestY])
			{ // Если встретили стену, то заканчиваем цикл
				if (step < 0.01)
					bHitWall = true;
				else
				{
					fDistanceToWall -= step;
					step /= 2.0f;
				}

				/*
				vector <pair <float, float>> p;

				for (int tx = 0; tx < 2; tx++)
				for (int ty = 0; ty < 2; ty++) // Проходим по всем 4м рёбрам
				{
				float vx = (float)nTestX + tx - fPlayerX; // Координаты вектора,
				float vy = (float)nTestY + ty - fPlayerY; // ведущего из наблюдателя в ребро
				float d = sqrt(vx*vx + vy*vy); // Модуль этого вектора
				float dot = (fEyeX*vx / d) + (fEyeY*vy / d); // Скалярное произведение (единичных векторов)
				p.push_back(make_pair(d, dot)); // Сохраняем результат в массив
				}
				// Мы будем выводить два ближайших ребра, поэтому сортируем их по модулю вектора ребра
				sort(p.begin(), p.end(), [](const pair <float, float> &left, const pair <float, float> &right) {return left.first < right.first; });

				float fBound = 0.00005; // Угол, при котором начинаем различать ребро.
				if (acos(p.at(0).second) < fBound) bBoundary = true;
				if (acos(p.at(1).second) < fBound) bBoundary = true;
				*/
			}
		}

		//Вычисляем координаты начала и конца стенки по формулам (1) и (2)
		float nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
		float nFloor = nScreenHeight - nCeiling;
		float maxfloorshade = 0.5f;
		float floorshade = maxfloorshade * (1.0f - nCeiling / (nScreenHeight / 2.0));

		float shade = 0;
		if (fDistanceToWall < fDepth)
			shade = 0.75 * (1 - fDistanceToWall / fDepth);

		if (nCeiling < 0)
			nCeiling = 0;
		if (nFloor > nScreenHeight)
			nFloor = nScreenHeight;

		// wall
		
		/*  bullshit
		int m = 255;
		float smudge = nCeiling + (nFloor - nCeiling) * (rand() % m) / (3.0f * m);

		glColor3f(shade/2.0, 0.0f, 0.0f);
		glVertex2f(x, nScreenHeight - nCeiling);
		glVertex2f(x, nScreenHeight - smudge);
		glColor3f(shade, shade, shade);
		glVertex2f(x, nScreenHeight - smudge);
		glVertex2f(x, nScreenHeight - nFloor);
		*/

		glColor3f(shade, shade, shade);
		glVertex2f(x, nScreenHeight - nCeiling);
		glVertex2f(x, nScreenHeight - nFloor);

		// floor
		glColor3f(floorshade, floorshade, floorshade);
		glVertex2f(x, nScreenHeight - nFloor);
		glColor3f(0.0f, maxfloorshade, maxfloorshade);
		//glColor3f(maxfloorshade, maxfloorshade, maxfloorshade);
		glVertex2f(x, 0);

		// ceiling
		glColor3f(floorshade, floorshade, 0.0f);
		//glColor3f(floorshade, floorshade, floorshade);
		glVertex2f(x, nScreenHeight - nCeiling);
		glColor3f(maxfloorshade, maxfloorshade, 0.0f);
		//glColor3f(maxfloorshade, maxfloorshade, maxfloorshade);
		glVertex2f(x, nScreenHeight);
	}

	glEnd();
	glutSwapBuffers();
}

int main(int argc, char* argv[])
{
	srand(time(0));

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(nScreenWidth, nScreenHeight);
	glutCreateWindow("Test");
	
	for (int i = 0; i < mapwidth; i++)
	{
		tiles.push_back(vector<bool>());
		for (int j = 0; j < mapheight; j++)
			tiles[i].push_back(true);
	}

	Leaf* dungeon = new Leaf(0, 0, mapwidth, mapheight);
	dungeon->drawHalls();
	dungeon->drawRoom();

	while (tiles[(int)fPlayerX][(int)fPlayerY])
	{
		fPlayerX = rand() % mapwidth;
		fPlayerY = rand() % mapheight;
	}

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);

	glutKeyboardFunc(action);
	glutIdleFunc(display);

	SetCursorPos(nScreenWidth / 2, nScreenHeight / 2);

	glutMainLoop();
	
	return 0;
}