#if defined(__GNUC__)
#pragma GCC optimize("O3")
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

static const int BoardH = 15;
static const int BoardW = 15;
static const int BoardArea = BoardH * BoardW;
static const int InputC = 43;
static const int FirstMoveX = 5;
static const int FirstMoveY = 7;

struct Move {
  int x0 = -1;
  int y0 = -1;
  int x1 = -1;
  int y1 = -1;
};

struct ThreatPair {
  int x0 = -1;
  int y0 = -1;
  int x1 = -1;
  int y1 = -1;
};

class ConnectSixNet {
public:
  int blocks = 0;
  int channels = 0;

  bool load(const string& path) {
    ifstream in(path.c_str(), ios::binary);
    if(!in)
      return false;

    char magic[4];
    readRaw(in, magic, 4);
    if(!(magic[0] == 'C' && magic[1] == '6' && magic[2] == 'R' && magic[3] == '1'))
      throw runtime_error("bad model magic");

    const int version = readI32(in);
    const int h = readI32(in);
    const int w = readI32(in);
    const int inputC = readI32(in);
    blocks = readI32(in);
    channels = readI32(in);

    if(version != 1 || h != BoardH || w != BoardW || inputC != InputC)
      throw runtime_error("unsupported model format");
    if(blocks <= 0 || channels <= 0)
      throw runtime_error("bad model shape");

    inputHead = readLayer(in);
    trunk.resize(static_cast<size_t>(blocks) * 2);
    for(size_t i = 0; i < trunk.size(); i++)
      trunk[i] = readLayer(in);
    outputHead = readLayer(in);

    valueWeight.resize(static_cast<size_t>(4) * channels);
    valueBias.resize(4);
    policyWeight.resize(channels);
    readF32(in, valueWeight);
    readF32(in, valueBias);
    readF32(in, policyWeight);
    return true;
  }

  void forward(
    const vector<float>& input,
    array<float, 3>& value,
    array<float, BoardArea>& policy
  ) const {
    vector<float> h;
    convRelu(input, inputHead, h);

    vector<float> tmp1;
    vector<float> tmp2;
    for(int b = 0; b < blocks; b++) {
      convRelu(h, trunk[static_cast<size_t>(2 * b)], tmp1);
      convRelu(tmp1, trunk[static_cast<size_t>(2 * b + 1)], tmp2);
      for(size_t i = 0; i < h.size(); i++)
        h[i] += tmp2[i];
    }

    convRelu(h, outputHead, tmp1);

    vector<float> means(channels, 0.0f);
    for(int c = 0; c < channels; c++) {
      float sum = 0.0f;
      const int base = c * BoardArea;
      for(int p = 0; p < BoardArea; p++)
        sum += tmp1[base + p];
      means[static_cast<size_t>(c)] = sum / static_cast<float>(BoardArea);
    }

    for(int o = 0; o < 3; o++) {
      float sum = valueBias[o];
      for(int c = 0; c < channels; c++)
        sum += valueWeight[static_cast<size_t>(o) * channels + c] * means[static_cast<size_t>(c)];
      value[static_cast<size_t>(o)] = sum;
    }

    for(int p = 0; p < BoardArea; p++) {
      float sum = 0.0f;
      for(int c = 0; c < channels; c++)
        sum += policyWeight[static_cast<size_t>(c)] * tmp1[static_cast<size_t>(c) * BoardArea + p];
      policy[static_cast<size_t>(p)] = sum;
    }
  }

private:
  struct Layer {
    int outC = 0;
    int inC = 0;
    int k = 0;
    vector<float> weight;
    vector<float> bias;
  };

  Layer inputHead;
  vector<Layer> trunk;
  Layer outputHead;
  vector<float> valueWeight;
  vector<float> valueBias;
  vector<float> policyWeight;

  static void readRaw(ifstream& in, char* dst, size_t bytes) {
    in.read(dst, static_cast<streamsize>(bytes));
    if(!in)
      throw runtime_error("unexpected end of model file");
  }

  static int readI32(ifstream& in) {
    int32_t value = 0;
    readRaw(in, reinterpret_cast<char*>(&value), sizeof(value));
    return static_cast<int>(value);
  }

  static void readF32(ifstream& in, vector<float>& dst) {
    readRaw(in, reinterpret_cast<char*>(dst.data()), dst.size() * sizeof(float));
  }

  static Layer readLayer(ifstream& in) {
    Layer layer;
    layer.outC = readI32(in);
    layer.inC = readI32(in);
    layer.k = readI32(in);
    if(layer.outC <= 0 || layer.inC <= 0 || layer.k <= 0)
      throw runtime_error("bad layer shape");
    layer.weight.resize(static_cast<size_t>(layer.outC) * layer.inC * layer.k * layer.k);
    layer.bias.resize(layer.outC);
    readF32(in, layer.weight);
    readF32(in, layer.bias);
    return layer;
  }

  static float getAt(const vector<float>& x, int c, int y, int xPos) {
    if(y < 0 || y >= BoardH || xPos < 0 || xPos >= BoardW)
      return 0.0f;
    return x[static_cast<size_t>(c) * BoardArea + y * BoardW + xPos];
  }

  static void convRelu(const vector<float>& input, const Layer& layer, vector<float>& output) {
    output.assign(static_cast<size_t>(layer.outC) * BoardArea, 0.0f);
    const int radius = layer.k / 2;

    for(int oc = 0; oc < layer.outC; oc++) {
      float* outBase = output.data() + static_cast<size_t>(oc) * BoardArea;
      fill(outBase, outBase + BoardArea, layer.bias[static_cast<size_t>(oc)]);

      for(int ic = 0; ic < layer.inC; ic++) {
        const float* inBase = input.data() + static_cast<size_t>(ic) * BoardArea;
        for(int ky = 0; ky < layer.k; ky++) {
          const int dy = ky - radius;
          const int yBegin = max(0, -dy);
          const int yEnd = min(BoardH, BoardH - dy);
          if(yBegin >= yEnd)
            continue;

          for(int kx = 0; kx < layer.k; kx++) {
            const int dx = kx - radius;
            const int xBegin = max(0, -dx);
            const int xEnd = min(BoardW, BoardW - dx);
            if(xBegin >= xEnd)
              continue;

            const size_t wi =
              (((static_cast<size_t>(oc) * layer.inC + ic) * layer.k + ky) * layer.k + kx);
            const float weight = layer.weight[wi];
            if(weight == 0.0f)
              continue;

            for(int y = yBegin; y < yEnd; y++) {
              const float* inRow = inBase + (y + dy) * BoardW + (xBegin + dx);
              float* outRow = outBase + y * BoardW + xBegin;
              for(int x = xBegin; x < xEnd; x++)
                outRow[x - xBegin] += weight * inRow[x - xBegin];
            }
          }
        }
      }

      for(int p = 0; p < BoardArea; p++)
        outBase[p] = outBase[p] > 0.0f ? outBase[p] : 0.0f;
    }
  }
};

static size_t skipWs(const string& s, size_t pos) {
  while(pos < s.size() && (s[pos] == ' ' || s[pos] == '\n' || s[pos] == '\r' || s[pos] == '\t'))
    pos++;
  return pos;
}

static bool parseJsonString(const string& s, size_t& pos, string& out) {
  pos = skipWs(s, pos);
  if(pos >= s.size() || s[pos] != '"')
    return false;
  pos++;
  out.clear();
  while(pos < s.size() && s[pos] != '"') {
    if(s[pos] == '\\' && pos + 1 < s.size())
      pos++;
    out.push_back(s[pos]);
    pos++;
  }
  if(pos >= s.size())
    return false;
  pos++;
  return true;
}

static bool parseJsonInt(const string& s, size_t& pos, int& out) {
  pos = skipWs(s, pos);
  int sign = 1;
  if(pos < s.size() && s[pos] == '-') {
    sign = -1;
    pos++;
  }
  if(pos >= s.size() || s[pos] < '0' || s[pos] > '9')
    return false;
  int value = 0;
  while(pos < s.size() && s[pos] >= '0' && s[pos] <= '9') {
    value = value * 10 + (s[pos] - '0');
    pos++;
  }
  out = sign * value;
  return true;
}

static vector<Move> parseMovesArray(const string& json, const string& name) {
  vector<Move> moves;
  const string key = "\"" + name + "\"";
  size_t pos = json.find(key);
  if(pos == string::npos)
    return moves;
  pos = json.find('[', pos + key.size());
  if(pos == string::npos)
    return moves;
  pos++;

  while(pos < json.size()) {
    pos = skipWs(json, pos);
    if(pos >= json.size() || json[pos] == ']')
      break;
    if(json[pos] == ',') {
      pos++;
      continue;
    }
    if(json[pos] != '{') {
      pos++;
      continue;
    }
    pos++;

    Move m;
    while(pos < json.size()) {
      pos = skipWs(json, pos);
      if(pos < json.size() && json[pos] == '}') {
        pos++;
        break;
      }
      if(pos < json.size() && json[pos] == ',') {
        pos++;
        continue;
      }

      string field;
      int value = 0;
      if(!parseJsonString(json, pos, field))
        break;
      pos = skipWs(json, pos);
      if(pos >= json.size() || json[pos] != ':')
        break;
      pos++;
      if(!parseJsonInt(json, pos, value))
        break;

      if(field == "x0")
        m.x0 = value;
      else if(field == "y0")
        m.y0 = value;
      else if(field == "x1")
        m.x1 = value;
      else if(field == "y1")
        m.y1 = value;
    }
    moves.push_back(m);
  }

  return moves;
}

class Board {
public:
  array<float, 2 * BoardArea> board{};
  pair<int, int> lastloc = make_pair(-1, -1);
  int stage = 1;

  void play(int color, int y, int x) {
    if(x < 0 || y < 0 || x >= BoardW || y >= BoardH)
      return;
    board[static_cast<size_t>(color) * BoardArea + y * BoardW + x] = 1.0f;
    if(stage == 0)
      lastloc = make_pair(y, x);
    else
      lastloc = make_pair(-1, -1);
    stage = 1 - stage;
  }

  bool isLegal(int x, int y) const {
    return 0 <= x && x < BoardW && 0 <= y && y < BoardH
      && at(0, y, x) == 0.0f && at(1, y, x) == 0.0f;
  }

  vector<float> getNNInput(int nextplayer) const {
    vector<float> nninput(static_cast<size_t>(InputC) * BoardArea, 0.0f);

    for(int p = 0; p < BoardArea; p++) {
      if(nextplayer == 0) {
        nninput[p] = board[p];
        nninput[BoardArea + p] = board[BoardArea + p];
      }
      else {
        nninput[p] = board[BoardArea + p];
        nninput[BoardArea + p] = board[p];
      }
    }

    if(stage == 1) {
      array<float, BoardArea> legalmoves{};
      for(int p = 0; p < BoardArea; p++)
        legalmoves[static_cast<size_t>(p)] = 1.0f - board[p] - board[BoardArea + p];

      if(lastloc.first != -1) {
        nninput[static_cast<size_t>(2) * BoardArea + lastloc.first * BoardW + lastloc.second] = 1.0f;
        const array<float, BoardArea> prv = getPriorityValueArray();
        const float lastpr = prv[static_cast<size_t>(lastloc.first * BoardW + lastloc.second)];
        for(int p = 0; p < BoardArea; p++) {
          if(prv[static_cast<size_t>(p)] < lastpr - 1e-10f)
            legalmoves[static_cast<size_t>(p)] = 0.0f;
        }
      }
      else {
        for(int p = 0; p < BoardArea; p++)
          nninput[static_cast<size_t>(4 + 2) * BoardArea + p] = 1.0f;
      }

      for(int p = 0; p < BoardArea; p++) {
        nninput[static_cast<size_t>(3) * BoardArea + p] = legalmoves[static_cast<size_t>(p)];
        nninput[static_cast<size_t>(4 + 0) * BoardArea + p] = 1.0f;
      }
    }

    for(int p = 0; p < BoardArea; p++)
      nninput[static_cast<size_t>(4 + 12) * BoardArea + p] = -0.3f;
    return nninput;
  }

  bool wouldWinAfter(int pla, int y, int x) const {
    if(!isLegal(x, y))
      return false;
    static const int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    for(int d = 0; d < 4; d++) {
      const int dy = dirs[d][0];
      const int dx = dirs[d][1];
      int count = 1;
      int yy = y + dy;
      int xx = x + dx;
      while(0 <= yy && yy < BoardH && 0 <= xx && xx < BoardW && at(pla, yy, xx) == 1.0f) {
        count++;
        yy += dy;
        xx += dx;
      }
      yy = y - dy;
      xx = x - dx;
      while(0 <= yy && yy < BoardH && 0 <= xx && xx < BoardW && at(pla, yy, xx) == 1.0f) {
        count++;
        yy -= dy;
        xx -= dx;
      }
      if(count >= 6)
        return true;
    }
    return false;
  }

  pair<int, int> findImmediateWin(int pla) const {
    for(int y = 0; y < BoardH; y++) {
      for(int x = 0; x < BoardW; x++) {
        if(wouldWinAfter(pla, y, x))
          return make_pair(x, y);
      }
    }
    return make_pair(-1, -1);
  }

  ThreatPair findWinningPair(int pla) const {
    ThreatPair best;
    double bestScore = -1e100;
    static const int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    for(int sy = 0; sy < BoardH; sy++) {
      for(int sx = 0; sx < BoardW; sx++) {
        for(int d = 0; d < 4; d++) {
          const int dy = dirs[d][0];
          const int dx = dirs[d][1];
          int countPla = 0;
          bool blocked = false;
          vector<pair<int, int> > empties;

          for(int k = 0; k < 6; k++) {
            const int y = sy + k * dy;
            const int x = sx + k * dx;
            if(y < 0 || y >= BoardH || x < 0 || x >= BoardW) {
              blocked = true;
              break;
            }
            if(at(1 - pla, y, x) == 1.0f) {
              blocked = true;
              break;
            }
            if(at(pla, y, x) == 1.0f)
              countPla++;
            else
              empties.push_back(make_pair(x, y));
          }

          if(blocked)
            continue;
          if(countPla == 4 && empties.size() == 2) {
            const int x0 = empties[0].first;
            const int y0 = empties[0].second;
            const int x1 = empties[1].first;
            const int y1 = empties[1].second;
            const double score = pointTieScore(x0, y0) + pointTieScore(x1, y1);
            if(score > bestScore) {
              bestScore = score;
              if(pointTieScore(x0, y0) >= pointTieScore(x1, y1)) {
                best.x0 = x0;
                best.y0 = y0;
                best.x1 = x1;
                best.y1 = y1;
              }
              else {
                best.x0 = x1;
                best.y0 = y1;
                best.x1 = x0;
                best.y1 = y0;
              }
            }
          }
        }
      }
    }

    return best;
  }

  pair<int, int> findPairBlock(int pla) const {
    array<int, BoardArea> cover{};
    static const int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    for(int sy = 0; sy < BoardH; sy++) {
      for(int sx = 0; sx < BoardW; sx++) {
        for(int d = 0; d < 4; d++) {
          const int dy = dirs[d][0];
          const int dx = dirs[d][1];
          int countPla = 0;
          bool blocked = false;
          vector<pair<int, int> > empties;

          for(int k = 0; k < 6; k++) {
            const int y = sy + k * dy;
            const int x = sx + k * dx;
            if(y < 0 || y >= BoardH || x < 0 || x >= BoardW) {
              blocked = true;
              break;
            }
            if(at(1 - pla, y, x) == 1.0f) {
              blocked = true;
              break;
            }
            if(at(pla, y, x) == 1.0f)
              countPla++;
            else
              empties.push_back(make_pair(x, y));
          }

          if(!blocked && countPla == 4 && empties.size() == 2) {
            const int idx0 = empties[0].second * BoardW + empties[0].first;
            const int idx1 = empties[1].second * BoardW + empties[1].first;
            cover[static_cast<size_t>(idx0)]++;
            cover[static_cast<size_t>(idx1)]++;
          }
        }
      }
    }

    int bestIndex = -1;
    int bestCover = 0;
    double bestScore = -1e100;
    for(int i = 0; i < BoardArea; i++) {
      if(cover[static_cast<size_t>(i)] <= 0)
        continue;
      const int x = i % BoardW;
      const int y = i / BoardW;
      const double score = pointTieScore(x, y);
      if(cover[static_cast<size_t>(i)] > bestCover
        || (cover[static_cast<size_t>(i)] == bestCover && score > bestScore)) {
        bestCover = cover[static_cast<size_t>(i)];
        bestScore = score;
        bestIndex = i;
      }
    }

    if(bestIndex == -1)
      return make_pair(-1, -1);
    return make_pair(bestIndex % BoardW, bestIndex / BoardW);
  }

  pair<int, int> findSimpleWin(int pla) const {
    const pair<int, int> direct = findImmediateWin(pla);
    if(direct.first != -1)
      return direct;

    static const int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    for(int x = 0; x < BoardW; x++) {
      for(int y = 0; y < BoardH; y++) {
        for(int d = 0; d < 4; d++) {
          const int dx = dirs[d][0];
          const int dy = dirs[d][1];
          int countPla = 0;
          vector<pair<int, int> > emptyPositions;

          for(int i = 0; i < 6; i++) {
            const int nx = x + i * dx;
            const int ny = y + i * dy;
            if(0 <= nx && nx < BoardW && 0 <= ny && ny < BoardH) {
              if(at(pla, ny, nx) == 1.0f)
                countPla++;
              else if(at(0, ny, nx) == 0.0f && at(1, ny, nx) == 0.0f)
                emptyPositions.push_back(make_pair(nx, ny));
            }
            else {
              break;
            }
          }

          if(!emptyPositions.empty()) {
            if((stage == 0 && countPla == 4 && emptyPositions.size() == 2)
              || (stage == 0 && countPla == 5 && emptyPositions.size() == 1)
              || (stage == 1 && countPla == 5 && emptyPositions.size() == 1)) {
              return emptyPositions[0];
            }
          }
        }
      }
    }

    return make_pair(-1, -1);
  }

  static double pointTieScore(int x, int y) {
    const double cx = (BoardW - 1) / 2.0;
    const double cy = (BoardH - 1) / 2.0;
    return -((x - cx) * (x - cx) + (y - cy) * (y - cy));
  }

private:
  float at(int color, int y, int x) const {
    return board[static_cast<size_t>(color) * BoardArea + y * BoardW + x];
  }

  array<float, BoardArea> getPriorityValueArray() const {
    array<float, BoardArea> distances{};
    if(lastloc.first == -1)
      return distances;

    float totalWeight = 0.0f;
    float xWeighted = 0.0f;
    float yWeighted = 0.0f;
    for(int y = 0; y < BoardH; y++) {
      for(int x = 0; x < BoardW; x++) {
        const float stones = at(0, y, x) + at(1, y, x);
        totalWeight += stones;
        xWeighted += x * stones;
        yWeighted += y * stones;
      }
    }
    if(totalWeight <= 0.0f)
      return distances;

    const float xCentroid = xWeighted / totalWeight;
    const float yCentroid = yWeighted / totalWeight;
    for(int y = 0; y < BoardH; y++) {
      for(int x = 0; x < BoardW; x++) {
        const float dx = x - xCentroid;
        const float dy = y - yCentroid;
        distances[static_cast<size_t>(y * BoardW + x)] = dx * dx + dy * dy;
      }
    }
    return distances;
  }
};

static void loadRequiredModel(ConnectSixNet& net) {
  if(!net.load("data/con6_10x64_cpp.bin"))
    throw runtime_error("model not found: data/con6_10x64_cpp.bin");
}

static pair<int, int> modelMove(const ConnectSixNet& net, const Board& board, int nextplayer, int movenum) {
  vector<float> nninput = board.getNNInput(nextplayer);
  array<float, 3> value{};
  array<float, BoardArea> policy{};
  net.forward(nninput, value, policy);

  float maxLogit = -numeric_limits<float>::infinity();
  for(int i = 0; i < BoardArea; i++) {
    const int x = i % BoardW;
    const int y = i / BoardW;
    if(board.isLegal(x, y))
      maxLogit = max(maxLogit, policy[static_cast<size_t>(i)]);
  }
  if(!isfinite(maxLogit))
    throw runtime_error("bad policy logits");

  const double policyTemp = 0.5 * pow(0.5, movenum / 10.0) + 0.01;
  array<double, BoardArea> weights{};
  double total = 0.0;
  for(int i = 0; i < BoardArea; i++) {
    const int x = i % BoardW;
    const int y = i / BoardW;
    if(!board.isLegal(x, y))
      continue;
    const double shifted = static_cast<double>(policy[static_cast<size_t>(i)] - maxLogit);
    if(shifted < -1.0)
      continue;
    const double weight = exp(shifted / policyTemp);
    if(isfinite(weight) && weight > 0.0) {
      weights[static_cast<size_t>(i)] = weight;
      total += weight;
    }
  }
  if(!(total > 0.0) || !isfinite(total))
    throw runtime_error("bad policy distribution");

  static mt19937 rng(static_cast<unsigned int>(
    chrono::high_resolution_clock::now().time_since_epoch().count()
  ));
  uniform_real_distribution<double> dist(0.0, total);
  double r = dist(rng);
  for(int i = 0; i < BoardArea; i++) {
    r -= weights[static_cast<size_t>(i)];
    if(r <= 0.0)
      return make_pair(i % BoardW, i / BoardW);
  }

  throw runtime_error("failed to sample policy");
}

static pair<int, int> tacticalMove(const Board& board, int nextplayer) {
  const int opp = 1 - nextplayer;

  pair<int, int> loc = board.findImmediateWin(nextplayer);
  if(loc.first != -1)
    return loc;

  if(board.stage == 0) {
    const ThreatPair pairWin = board.findWinningPair(nextplayer);
    if(pairWin.x0 != -1)
      return make_pair(pairWin.x0, pairWin.y0);
  }

  loc = board.findImmediateWin(opp);
  if(loc.first != -1)
    return loc;

  loc = board.findPairBlock(opp);
  if(loc.first != -1)
    return loc;

  loc = board.findSimpleWin(nextplayer);
  if(loc.first != -1)
    return loc;

  return make_pair(-1, -1);
}

static void printResponse(int x0, int y0, int x1, int y1) {
  cout << "{\"response\":{\"x0\":" << x0
       << ",\"y0\":" << y0
       << ",\"x1\":" << x1
       << ",\"y1\":" << y1
       << "}}" << endl;
}

int main() {
  ios::sync_with_stdio(false);
  cin.tie(0);

  ConnectSixNet net;
  loadRequiredModel(net);

  string input;
  {
    string line;
    while(getline(cin, line))
      input += line;
  }

  vector<Move> requests = parseMovesArray(input, "requests");
  vector<Move> responses = parseMovesArray(input, "responses");
  if(requests.empty()) {
    printResponse(FirstMoveX, FirstMoveY, -1, -1);
    return 0;
  }

  const Move& lastRequest = requests.back();
  if(lastRequest.x0 == -1) {
    printResponse(FirstMoveX, FirstMoveY, -1, -1);
    return 0;
  }

  const int nextPlayer = (requests[0].x0 != -1 && requests[0].x1 == -1) ? 1 : 0;
  const int opp = 1 - nextPlayer;

  Board board;
  for(size_t i = 0; i < requests.size(); i++) {
    const Move& r = requests[i];
    if(r.x0 != -1)
      board.play(opp, r.y0, r.x0);
    if(r.x1 != -1)
      board.play(opp, r.y1, r.x1);

    if(i < responses.size()) {
      const Move& resp = responses[i];
      if(resp.x0 != -1)
        board.play(nextPlayer, resp.y0, resp.x0);
      if(resp.x1 != -1)
        board.play(nextPlayer, resp.y1, resp.x1);
    }
  }

  int outX[2] = {-1, -1};
  int outY[2] = {-1, -1};

  for(int stage = 0; stage < 2; stage++) {
    pair<int, int> loc = tacticalMove(board, nextPlayer);
    if(loc.first == -1)
      loc = modelMove(net, board, nextPlayer, static_cast<int>(requests.size()) * 2 + stage);

    int actionX = loc.first;
    int actionY = loc.second;
    if(actionX == -1 || !board.isLegal(actionX, actionY))
      throw runtime_error("selected illegal move");

    outX[stage] = actionX;
    outY[stage] = actionY;
    board.play(nextPlayer, actionY, actionX);
  }

  printResponse(outX[0], outY[0], outX[1], outY[1]);
  return 0;
}