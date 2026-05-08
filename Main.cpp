#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <limits>
#include <set>
#include <queue>
#include <map>
#include <string>

using namespace std;

const int U = 200;
const int N = 15000;
const int RUNS = 20;
const double Tstart = 1000.0;
const double Tend = 0.1;
const double alpha = 0.995;
const int C = 150;
const int MAX_GIFTS = 500;

vector<string> actions;
vector<long> scoreHistory;

struct Gift {
	string name;
	int id;
	int score;
	int mass;
	int c, r;

	static int nextId;
	Gift(string name, int score, int mass, int c, int r) :name(name), score(score), mass(mass), c(c), r(r) {
		id = nextId++;
	}
};
int Gift::nextId = 0;

struct State {
	long long x = 0, y = 0;
	long long vx = 0, vy = 0;
	int time = 0;
	long long weight = 0;
	int carrots = 0;
};

enum Dir { NONE, UP, DOWN, LEFT, RIGHT };

struct Accel {
	Dir dir = NONE;
	int a = 0;
};

typedef struct Point {
	long long x;
	long long y;
	Point(long long x, long long y) {
		this->x = x;
		this->y = y;
	}
} Point;

int T, D, W, G, K;
vector<Gift> gifts;
vector<pair<int, int>> accelTiers;
set<int> globallyUsed;
vector<Point> points;
vector<Point> giftsUsed;

inline long double dist2(long long x1, long long y1, long long x2, long long y2) {
	long long dx = x1 - x2;
	long long dy = y1 - y2;
	return dx * dx + dy * dy;
}

vector<Gift> sortByAngle(vector<Gift> gifts) {

	sort(gifts.begin(), gifts.end(), [](const Gift& a, const Gift& b) {

		double angleA = atan2(a.r, a.c);
		double angleB = atan2(b.r, b.c);

		if (angleA != angleB)
			return angleA < angleB;

		return dist2(0, 0, a.c, a.r) < dist2(0, 0, b.c, b.r);
		});

	return gifts;
}

vector<Gift> sortByClusters(vector<Gift> gifts) {
	const int GRID = 20000;
	map<pair<int, int>, vector<Gift>> clusters;

	//grupisanje u klastere
	for (auto& g : gifts) {
		int cx = g.c / GRID;
		int cy = g.r / GRID;
		clusters[{cx, cy}].push_back(g);
	}

	//sortiranje klastera po udaljenosti centra klastera od (0,0)
	//ovo osigurava da Deda Mraz ide prvo ka najbližim grupama poklona
	vector<pair<pair<int, int>, vector<Gift>>> sortedClusters(clusters.begin(), clusters.end());
	sort(sortedClusters.begin(), sortedClusters.end(), [](const auto& a, const auto& b) {
		long long distA = (long long)a.first.first * a.first.first + (long long)a.first.second * a.first.second;
		long long distB = (long long)b.first.first * b.first.first + (long long)b.first.second * b.first.second;
		return distA < distB;
		});

	vector<Gift> result;
	long long lastX = 0, lastY = 0; //pocinjemo od baze

	//primena Nearest Neighbor unutar svakog klastera
	for (auto& clusterItem : sortedClusters) {
		vector<Gift>& clusterGifts = clusterItem.second;

		while (!clusterGifts.empty()) {
			long long minDist = LLONG_MAX;
			int nextIdx = -1;

			for (int i = 0; i < clusterGifts.size(); i++) {
				long long d = dist2(lastX, lastY, clusterGifts[i].c, clusterGifts[i].r);
				if (d < minDist) {
					minDist = d;
					nextIdx = i;
				}
			}

			if (nextIdx != -1) {
				result.push_back(clusterGifts[nextIdx]);
				lastX = clusterGifts[nextIdx].c;
				lastY = clusterGifts[nextIdx].r;
				clusterGifts.erase(clusterGifts.begin() + nextIdx);
			}
		}
	}

	return result;
}

int getMaxAccel(long long weight) {
	for (auto& t : accelTiers) {
		if (weight <= t.first)
			return t.second;
	}
	return 0;
}

int getRandom(int a, int b) {
	static random_device rd;
	static mt19937 rng(rd());
	uniform_int_distribution<int> dist(a, b);
	return dist(rng);
}

//radi po principu trazenja Nearest Neighbor
vector<Gift> sortByPath(vector<Gift>& initial) {

	int n = initial.size();

	vector<bool> used(n, false);
	vector<Gift> result;
	result.reserve(n);

	//nadji prvi najblizi (0, 0)
	int idx = min_element(initial.begin(), initial.end(),
		[](const Gift& a, const Gift& b) {
			return dist2(0, 0, a.c, a.r) < dist2(0, 0, b.c, b.r);
		}) - initial.begin();

	result.push_back(initial[idx]);
	used[idx] = true;

	//nadji prvi najblizi prvom, zatim najblizi drugom...
	for (int i = 1; i < n; i++) {

		long long minDist = LLONG_MAX;
		int nextIdx = -1;

		const Gift& last = result.back();

		for (int j = 0; j < n; j++) {
			if (!used[j]) {
				long long d = dist2(last.c, last.r,
					initial[j].c, initial[j].r);

				if (d < minDist) {
					minDist = d;
					nextIdx = j;
				}
			}
		}

		if (nextIdx == -1) break;

		result.push_back(initial[nextIdx]);
		used[nextIdx] = true;
	}

	return result;
}

void readFile(string filename) {
	ifstream file(filename);

	if (!file) {
		cerr << "Greska sa fajlom" << endl;
		return;
	}

	string line;

	file >> T >> D >> W >> G;

	if (T < 1 || T > 10000) {
		cerr << "T parametar van dozvoljenih granica" << endl;
		return;
	}
	else if (D < 0 || D > 100) {
		cerr << "D parametar van dozvoljenih granica" << endl;
		return;
	}
	else if (W < 1 || W > 10) {
		cerr << "W parametar van dozvoljenih granica" << endl;
		return;
	}
	else if (G < 1 || G > 10000) {
		cerr << "G parametar van dozvoljenih granica" << endl;
		return;
	}

	for (int i = 0; i < W; i++) {
		int w1, a1;

		file >> w1;
		file >> a1;

		accelTiers.emplace_back(w1, a1);
	}

	for (int i = 0; i < G; i++) {
		string name;
		int score, c, r;
		long mass;
		file >> name >> score >> mass >> c >> r;
		gifts.push_back({ name, score, mass, c, r });
	}

	gifts = sortByPath(gifts);
	//gifts = sortByAngle(gifts);
	//gifts = sortByClusters(gifts);

	K = min(G, MAX_GIFTS);
}

void neighbor(vector<int>& x, int u) {
	int idx = getRandom(0, u - 1);
	x[idx] = 1 - x[idx];
}

void collectGifts(vector<Gift>& initial, set<int>& used, set<int>& globallyUsed) {

	int remaining = G - globallyUsed.size();
	int limit = min(K, remaining);

	int idx = 0;

	while (initial.size() < limit && idx < G) {

		if (!globallyUsed.count(gifts[idx].id)) {

			used.insert(idx);
			globallyUsed.insert(gifts[idx].id);

			initial.push_back(gifts[idx]);
		}

		idx++;
	}
}

void pickGifts(vector<int>& x, vector<int>& toVisit, int u, set<int>& visited, vector<Gift>& initial) {
	toVisit.clear();
	x.resize(u);

	for (int i = 0; i < u; i++) {

		if (visited.count(initial[i].id)) {
			x[i] = 0;
			continue;
		}

		x[i] = getRandom(0, 1);

		if (x[i])
			toVisit.push_back(i);
	}
}

Accel steer(State& s, long long tx, long long ty) {
	Accel act;
	int maxA = getMaxAccel(s.weight);
	if (maxA == 0) return act;

	long long dx = tx - s.x;
	long long dy = ty - s.y;

	//odlucujemo na kojoj osi radimo korekciju brzine (prioritet je veca udaljenost)
	if (abs(dx) > abs(dy)) {
		//koliko nam treba da se potpuno zaustavimo sa trenutne vx
		//d = (v^2) / (2 * a) => a_potrebno = (v^2) / (2 * d)
		long long stopDist = (s.vx * s.vx) / (2.0 * maxA);

		if (abs(dx) > stopDist + D / 2) {
			//ako smo daleko, ubrzaj ka cilju
			act.dir = dx > 0 ? RIGHT : LEFT;
			act.a = maxA;
		}
		else {
			//ako smo blizu zone kocenja, precizno doziraj usporavanje
			if (s.vx > 0) {
				act.dir = LEFT;
				act.a = min((long long)maxA, (long long)abs(s.vx)); //koci samo onoliko kolika je brzina
			}
			else if (s.vx < 0) {
				act.dir = RIGHT;
				act.a = min((long long)maxA, (long long)abs(s.vx));
			}
		}
	}
	else {
		long long stopDist = (s.vy * s.vy) / (2.0 * maxA);

		if (abs(dy) > stopDist + D / 2) {
			act.dir = dy > 0 ? UP : DOWN;
			act.a = maxA;
		}
		else {
			if (s.vy > 0) {
				act.dir = DOWN;
				act.a = min((long long)maxA, (long long)abs(s.vy));
			}
			else if (s.vy < 0) {
				act.dir = UP;
				act.a = min((long long)maxA, (long long)abs(s.vy));
			}
		}
	}

	if (act.a == 0) act.dir = NONE;
	return act;
}

long long evaluate(vector<int>& x, vector<Gift>& initial, int u) {
	long long score = 0;
	int deliveredCnt = 0;

	points.clear();
	giftsUsed.clear();
	actions.clear();

	vector<int> toVisit;
	set<int> visited;
	for (int i = 0; i < u; i++) {
		if (x[i])
			toVisit.push_back(i);
	}

	State santa;

	santa.x = 0;
	santa.y = 0;
	santa.vx = 0;
	santa.vy = 0;
	santa.time = 0;
	santa.weight = 0;

	for (int idx : toVisit) {
		santa.weight += initial[idx].mass;
		actions.push_back("LoadGift " + initial[idx].name);
		giftsUsed.push_back({ initial[idx].c, initial[idx].r });
	}

	santa.carrots = min(int(G / 2), C);
	santa.weight += santa.carrots;
	actions.push_back("LoadCarrots " + santa.carrots);

	while (santa.time < T) {
		if (santa.carrots == 0 || santa.weight == 0) {
			break;
		}
		if (toVisit.empty() && santa.carrots > 0) {
			cout << "Nova tura potrebna" << endl;
			Accel act = steer(santa, 0, 0);
			
			while (santa.time < T) {
				if (act.dir != NONE && act.a > 0) {
					string move;
					if (act.dir == UP) {
						santa.vy += act.a;
						move = "AccUp " + act.a;
					}
					else if (act.dir == DOWN) {
						santa.vy -= act.a;
						move = "AccDown " + act.a;
					}
					else if (act.dir == RIGHT) {
						santa.vx += act.a;
						move = "AccRight " + act.a;
					}
					else if (act.dir == LEFT) {
						santa.vx -= act.a;
						move = "AccLeft " + act.a;
					}

					actions.push_back(move);

					santa.weight -= 1;
					santa.carrots -= 1;
				}
				
				actions.push_back("Float 1");

				santa.x += santa.vx;
				santa.y += santa.vy;

				santa.time++;
				long long distToBase = dist2(0, 0, santa.x, santa.y);
				if (distToBase <= D*D) {
					break;
				}
			}
			santa.carrots += C;
			santa.weight += C;
			pickGifts(x, toVisit, u, visited, initial);
			if (toVisit.empty())
				break;
			for (int idx : toVisit) {
				santa.weight += initial[idx].mass;
				actions.push_back("LoadGift " + initial[idx].name);
				giftsUsed.push_back({ initial[idx].c, initial[idx].r });
			}
		}
		int next = toVisit.front();
		Gift curr = initial[next];
		while (true) {
			Accel act = steer(santa, curr.c, curr.r);
			if (act.dir != NONE && act.a > 0) {
				string move;
				if (act.dir == UP) {
					santa.vy += act.a;
					move = "AccUp ";
				}
				else if (act.dir == DOWN) {
					santa.vy -= act.a;
					move = "AccDown ";
				}
				else if (act.dir == RIGHT) {
					santa.vx += act.a;
					move = "AccDown ";
				}
				else if (act.dir == LEFT) {
					santa.vx -= act.a;
					move = "AccLeft ";
				}
				
				move += to_string(act.a);

				actions.push_back(move);

				santa.weight -= 1;
				santa.carrots -= 1;
			}
			actions.push_back("Float 1");

			santa.time++;

			if (santa.time >= T)
				break;

			points.push_back({ santa.x, santa.y });

			santa.x += santa.vx;
			santa.y += santa.vy;
			long long dist = dist2(curr.c, curr.r, santa.x, santa.y);

			if (dist <= D*D) {
				break;
			}
		}

		visited.insert(curr.id);
		score += curr.score;
		deliveredCnt++;
		actions.push_back("DeliverGift " + curr.name);
		santa.weight -= curr.mass;
		toVisit.erase(toVisit.begin());

		for (int i = 0; i < toVisit.size(); i++) {
			Gift temp = initial[toVisit[i]];
			if (dist2(santa.x, santa.y, temp.c, temp.r) <= D*D) {
				visited.insert(temp.id);
				score += temp.score;
				deliveredCnt++;
				actions.push_back("DeliverGift " + temp.name);
				santa.weight -= temp.mass;
				toVisit.erase(toVisit.begin() + i);
				i--;
			}
		}

		santa.time++;
		if (toVisit.empty())
			break;
	}
	return score;
}

void simulatedAnnealing() {
	set<int> used;
	vector<Gift> initial;
	vector<int> x;
	vector<int> bestX;
	vector<int> scores;
	vector<Point> bestPath;
	vector<Point> bestPathGifts;
	vector<string> bestActions;
	long long bestScore = -1;
	long long currentBestInRun;

	ofstream fiout("maksimumi.txt");

	for (int r = 0; r < RUNS; r++) {

		cout << "Run: " << r + 1 << endl;

		set<int> visited;
		vector<long> currentScoreHistory;

		used.clear();
		initial.clear();
		globallyUsed.clear();

		collectGifts(initial, used, globallyUsed); //pokupi poklone

		int u = min((int)initial.size(), U);

		x.assign(u, 0);

		for (int i = 0; i < u; i++)
			x[i] = getRandom(0, 1);

		long long currentScore = evaluate(x, initial, u);
		bestPath = points;
		bestPathGifts = giftsUsed;
		bestActions = actions;

		bestScore = currentScore;
		bestX = x;
		currentBestInRun = bestScore;

		double Tcur = Tstart;
		for (int it = 0; it < N && Tcur > Tend; it++) {
			vector<int> newX = x;
			neighbor(newX, u);

			long long newScore = evaluate(newX, initial, u);
			long long delta = newScore - currentScore;

			if (delta > 0) {
				x = newX;
				currentScore = newScore;
			}
			else {

				double prob = exp((double)delta / Tcur);

				double rnd = (double)getRandom(0, 1000000) / 1000000.0;

				if (rnd < prob) {
					x = newX;
					currentScore = newScore;
				}
			}

			if (currentScore > bestScore) {
				bestScore = currentScore;
				bestX = x;
				bestPath = points;
				bestPathGifts = giftsUsed;
				bestActions = actions;
			}

			if (currentScore > currentBestInRun)
				currentBestInRun = currentScore;

			currentScoreHistory.push_back(currentBestInRun);

			Tcur *= alpha;
		}

		for (const auto& i : currentScoreHistory) {
			fiout << i << endl;
		}
		fiout << endl;

		cout << "Best score: " << bestScore << endl;
		scores.push_back(bestScore);
	}

	long long finalBestScore = *max_element(scores.begin(), scores.end());
	
	cout << "Final best score: " << finalBestScore << endl;

	//ispis putanje
	ofstream pts("points.txt");
	for (const auto& p : points) {
		pts << p.x << " " << p.y << endl;
	}
	pts.close();

	ofstream actionsFile("actions.txt");
	actionsFile << "Highest score: " << finalBestScore << endl << endl;

	//ispis poklona koriscenih pri toj putanji
	ofstream g("giftsUsed.txt");
	for (const auto& it : bestPathGifts) {
		g << it.x << " " << it.y << endl;
	}
	g.close();

	//ispit akcija u odvojen fajl
	actionsFile << actions.size() << endl;
	for (const auto& a : bestActions) {
		actionsFile << a << endl;
	}

	fiout.close();
}

int main() {
	//readFile("a_an_example.in.txt");
	readFile("b_better_hurry.in.txt");
	//readFile("c_carousel.in.txt");
	//readFile("d_decorated_houses.in.txt");
	simulatedAnnealing();
	return 0;
}