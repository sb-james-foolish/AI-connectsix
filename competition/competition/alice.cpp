#if defined(__GNUC__)
#pragma GCC optimize("O3")
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
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

class KataStyleNet {
public:
  int modelVersion = 0;
  int spatialC = 0;
  int globalC = 0;
  int blocks = 0;
  int channels = 0;

  bool load(const string& path) {
    BinReader r(path);
    if(!r.ok())
      return false;

    modelName = r.token();
    if(modelName.rfind("connectsix", 0) != 0 && modelName.rfind("con6", 0) != 0)
      return false;

    modelVersion = r.i32();
    spatialC = r.i32();
    globalC = r.i32();

    const string trunkTok = r.token();
    if(trunkTok != "trunk")
      throw runtime_error("expected trunk");
    blocks = r.i32();
    channels = r.i32();
    const int midChannels = r.i32();
    const int regularChannels = r.i32();
    const int dilatedChannels = r.i32();
    const int gpoolChannels = r.i32();
    (void)midChannels;
    (void)regularChannels;
    (void)dilatedChannels;
    (void)gpoolChannels;

    initialConv = readConv(r);
    initialMat = readMat(r);

    trunk.clear();
    for(int b = 0; b < blocks; b++) {
      Block block;
      block.kind = r.token();
      block.name = r.token();
      if(block.kind == "ordinary_block") {
        block.pre1 = readBN(r);
        block.pre1.act = readAct(r);
        block.conv1 = readConv(r);
        block.pre2 = readBN(r);
        block.pre2.act = readAct(r);
        block.conv2 = readConv(r);
      }
      else if(block.kind == "gpool_block") {
        block.pre1 = readBN(r);
        block.pre1.act = readAct(r);
        block.conv1 = readConv(r);
        block.gconv = readConv(r);
        block.gbn = readBN(r);
        block.gbn.act = readAct(r);
        block.gmat = readMat(r);
        block.pre2 = readBN(r);
        block.pre2.act = readAct(r);
        block.conv2 = readConv(r);
      }
      else {
        throw runtime_error("unsupported block kind: " + block.kind);
      }
      trunk.push_back(std::move(block));
    }

    trunkTip = readBN(r);
    trunkTip.act = readAct(r);

    string tok = r.token();
    if(tok != "model.policy_head")
      throw runtime_error("expected policy head");
    policy.p1 = readConv(r);
    policy.g1 = readConv(r);
    policy.gbn = readBN(r);
    policy.gbn.act = readAct(r);
    policy.gmatBias = readMat(r);
    policy.pbn = readBN(r);
    policy.pbn.act = readAct(r);
    policy.p2 = readConv(r);
    policy.gmatPass = readMat(r);
    loaded = true;
    return true;
  }

  bool isLoaded() const {
    return loaded;
  }

  void forwardPolicy(
    const vector<float>& spatial,
    const vector<float>& global,
    array<float, BoardArea>& policyOut
  ) const {
    if(!loaded)
      throw runtime_error("kata-style model is not loaded");
    if((int)spatial.size() != spatialC * BoardArea || (int)global.size() != globalC)
      throw runtime_error("bad kata-style input size");

    vector<float> h;
    conv(spatial, initialConv, h);
    vector<float> gBias = matVec(initialMat, global);
    for(int c = 0; c < channels; c++) {
      float b = gBias[static_cast<size_t>(c)];
      float* base = h.data() + static_cast<size_t>(c) * BoardArea;
      for(int p = 0; p < BoardArea; p++)
        base[p] += b;
    }

    vector<float> tmp1, tmp2, tmp3;
    for(const Block& block : trunk) {
      tmp1 = h;
      applyBNAct(tmp1, block.pre1);
      conv(tmp1, block.conv1, tmp2);

      if(block.kind == "gpool_block") {
        conv(tmp1, block.gconv, tmp3);
        applyBNAct(tmp3, block.gbn);
        vector<float> gp = globalPool(tmp3, block.gbn.c);
        vector<float> bias = matVec(block.gmat, gp);
        for(int c = 0; c < block.conv1.outC; c++) {
          float b = bias[static_cast<size_t>(c)];
          float* base = tmp2.data() + static_cast<size_t>(c) * BoardArea;
          for(int p = 0; p < BoardArea; p++)
            base[p] += b;
        }
      }

      applyBNAct(tmp2, block.pre2);
      conv(tmp2, block.conv2, tmp1);
      for(size_t i = 0; i < h.size(); i++)
        h[i] += tmp1[i];
    }

    applyBNAct(h, trunkTip);

    vector<float> p1, g1;
    conv(h, policy.p1, p1);
    conv(h, policy.g1, g1);
    applyBNAct(g1, policy.gbn);
    vector<float> gp = globalPool(g1, policy.gbn.c);
    vector<float> pBias = matVec(policy.gmatBias, gp);
    for(int c = 0; c < policy.p1.outC; c++) {
      float b = pBias[static_cast<size_t>(c)];
      float* base = p1.data() + static_cast<size_t>(c) * BoardArea;
      for(int p = 0; p < BoardArea; p++)
        base[p] += b;
    }
    applyBNAct(p1, policy.pbn);
    vector<float> logits;
    conv(p1, policy.p2, logits);
    for(int p = 0; p < BoardArea; p++)
      policyOut[static_cast<size_t>(p)] = logits[static_cast<size_t>(p)];
  }

private:
  struct Conv {
    int ky = 0, kx = 0, inC = 0, outC = 0, dy = 1, dx = 1;
    vector<float> w; // oc,ic,y,x order
  };
  struct BN {
    int c = 0;
    int act = 1;
    vector<float> scale;
    vector<float> bias;
  };
  struct Mat {
    int inC = 0, outC = 0;
    vector<float> w; // in,out file order, accessed as in-major
  };
  struct Block {
    string kind;
    string name;
    BN pre1, pre2, gbn;
    Conv conv1, conv2, gconv;
    Mat gmat;
  };
  struct Policy {
    Conv p1, g1, p2;
    BN gbn, pbn;
    Mat gmatBias, gmatPass;
  };

  class BinReader {
  public:
    explicit BinReader(const string& path) : in(path.c_str(), ios::binary) {}
    bool ok() const { return (bool)in; }
    string token() {
      string s;
      in >> s;
      if(!in)
        throw runtime_error("unexpected end of model");
      return s;
    }
    int i32() { return atoi(token().c_str()); }
    float f32Text() { return static_cast<float>(atof(token().c_str())); }
    void readFloats(size_t n, vector<float>& dst, const string& name) {
      dst.resize(n);
      int skipped = 0;
      char ch = 0;
      do {
        if(!in.get(ch))
          throw runtime_error("missing @BIN@ for " + name);
        skipped++;
        if(skipped > 100)
          throw runtime_error("bad @BIN@ spacing for " + name);
      } while(ch != '@');
      char marker[4];
      in.read(marker, 4);
      if(!in || marker[0] != 'B' || marker[1] != 'I' || marker[2] != 'N' || marker[3] != '@')
        throw runtime_error("bad @BIN@ marker for " + name);
      in.read(reinterpret_cast<char*>(dst.data()), static_cast<streamsize>(n * sizeof(float)));
      if(!in)
        throw runtime_error("short binary float block for " + name);
    }
  private:
    ifstream in;
  };

  string modelName;
  bool loaded = false;
  Conv initialConv;
  Mat initialMat;
  vector<Block> trunk;
  BN trunkTip;
  Policy policy;

  static Conv readConv(BinReader& r) {
    Conv c;
    string name = r.token();
    c.ky = r.i32();
    c.kx = r.i32();
    c.inC = r.i32();
    c.outC = r.i32();
    c.dy = r.i32();
    c.dx = r.i32();
    vector<float> fileW;
    r.readFloats(static_cast<size_t>(c.ky) * c.kx * c.inC * c.outC, fileW, name);
    c.w.assign(fileW.size(), 0.0f);
    size_t src = 0;
    for(int y = 0; y < c.ky; y++)
      for(int x = 0; x < c.kx; x++)
        for(int ic = 0; ic < c.inC; ic++)
          for(int oc = 0; oc < c.outC; oc++)
            c.w[((static_cast<size_t>(oc) * c.inC + ic) * c.ky + y) * c.kx + x] = fileW[src++];
    return c;
  }

  static BN readBN(BinReader& r) {
    BN b;
    string name = r.token();
    b.c = r.i32();
    float eps = r.f32Text();
    int hasScale = r.i32();
    int hasBias = r.i32();
    vector<float> mean, var, scale, bias;
    r.readFloats(b.c, mean, name + ".mean");
    r.readFloats(b.c, var, name + ".var");
    if(hasScale) r.readFloats(b.c, scale, name + ".scale");
    else scale.assign(static_cast<size_t>(b.c), 1.0f);
    if(hasBias) r.readFloats(b.c, bias, name + ".bias");
    else bias.assign(static_cast<size_t>(b.c), 0.0f);
    b.scale.resize(static_cast<size_t>(b.c));
    b.bias.resize(static_cast<size_t>(b.c));
    for(int i = 0; i < b.c; i++) {
      b.scale[static_cast<size_t>(i)] = scale[static_cast<size_t>(i)] / sqrtf(var[static_cast<size_t>(i)] + eps);
      b.bias[static_cast<size_t>(i)] = bias[static_cast<size_t>(i)] - b.scale[static_cast<size_t>(i)] * mean[static_cast<size_t>(i)];
    }
    return b;
  }

  static Mat readMat(BinReader& r) {
    Mat m;
    string name = r.token();
    m.inC = r.i32();
    m.outC = r.i32();
    r.readFloats(static_cast<size_t>(m.inC) * m.outC, m.w, name);
    return m;
  }

  static int readAct(BinReader& r) {
    string name = r.token();
    string act = r.token();
    (void)name;
    if(act == "ACTIVATION_IDENTITY")
      return 0;
    if(act == "ACTIVATION_MISH")
      return 1;
    if(act == "ACTIVATION_RELU")
      return 2;
    throw runtime_error("unsupported activation: " + act);
  }

  static float mish(float x) {
    if(x > 20.0f) return x;
    if(x < -20.0f) return 0.0f;
    return x * tanhf(log1pf(expf(x)));
  }

  static void applyBNAct(vector<float>& x, const BN& b) {
    for(int c = 0; c < b.c; c++) {
      float* base = x.data() + static_cast<size_t>(c) * BoardArea;
      const float s = b.scale[static_cast<size_t>(c)];
      const float bias = b.bias[static_cast<size_t>(c)];
      for(int p = 0; p < BoardArea; p++) {
        const float v = base[p] * s + bias;
        if(b.act == 0)
          base[p] = v;
        else if(b.act == 2)
          base[p] = v > 0.0f ? v : 0.0f;
        else
          base[p] = mish(v);
      }
    }
  }

  static vector<float> matVec(const Mat& m, const vector<float>& x) {
    vector<float> y(static_cast<size_t>(m.outC), 0.0f);
    for(int ic = 0; ic < m.inC; ic++) {
      const float xv = x[static_cast<size_t>(ic)];
      const float* row = m.w.data() + static_cast<size_t>(ic) * m.outC;
      for(int oc = 0; oc < m.outC; oc++)
        y[static_cast<size_t>(oc)] += xv * row[oc];
    }
    return y;
  }

  static vector<float> globalPool(const vector<float>& x, int cNum) {
    vector<float> out(static_cast<size_t>(cNum) * 3, 0.0f);
    for(int c = 0; c < cNum; c++) {
      const float* base = x.data() + static_cast<size_t>(c) * BoardArea;
      float sum = 0.0f;
      float mx = -numeric_limits<float>::infinity();
      for(int p = 0; p < BoardArea; p++) {
        sum += base[p];
        mx = max(mx, base[p]);
      }
      const float mean = sum / static_cast<float>(BoardArea);
      const float scaledMean = mean * ((sqrtf(static_cast<float>(BoardArea)) - 14.0f) / 10.0f);
      out[static_cast<size_t>(c)] = mean;
      out[static_cast<size_t>(cNum) + c] = scaledMean;
      out[static_cast<size_t>(2 * cNum) + c] = mx;
    }
    return out;
  }

  static void conv(const vector<float>& input, const Conv& layer, vector<float>& output) {
    output.assign(static_cast<size_t>(layer.outC) * BoardArea, 0.0f);
    const int ry = layer.ky / 2;
    const int rx = layer.kx / 2;
    for(int oc = 0; oc < layer.outC; oc++) {
      float* outBase = output.data() + static_cast<size_t>(oc) * BoardArea;
      for(int ic = 0; ic < layer.inC; ic++) {
        const float* inBase = input.data() + static_cast<size_t>(ic) * BoardArea;
        for(int ky = 0; ky < layer.ky; ky++) {
          const int offY = (ky - ry) * layer.dy;
          const int yBegin = max(0, -offY);
          const int yEnd = min(BoardH, BoardH - offY);
          if(yBegin >= yEnd) continue;
          for(int kx = 0; kx < layer.kx; kx++) {
            const int offX = (kx - rx) * layer.dx;
            const int xBegin = max(0, -offX);
            const int xEnd = min(BoardW, BoardW - offX);
            if(xBegin >= xEnd) continue;
            const float wt = layer.w[((static_cast<size_t>(oc) * layer.inC + ic) * layer.ky + ky) * layer.kx + kx];
            if(wt == 0.0f) continue;
            for(int y = yBegin; y < yEnd; y++) {
              const float* inRow = inBase + (y + offY) * BoardW + (xBegin + offX);
              float* outRow = outBase + y * BoardW + xBegin;
              for(int x = xBegin; x < xEnd; x++)
                outRow[x - xBegin] += wt * inRow[x - xBegin];
            }
          }
        }
      }
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
        const array<double, BoardArea> prv = getPriorityValueArray();
        const double lastpr = prv[static_cast<size_t>(lastloc.first * BoardW + lastloc.second)];
        for(int p = 0; p < BoardArea; p++) {
          if(prv[static_cast<size_t>(p)] < lastpr - 1e-10)
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

  void getKataInput(int nextplayer, vector<float>& spatial, vector<float>& global) const {
    static const int KataSpatialC = 22;
    static const int KataGlobalC = 39;
    static const double PriorEps = 1e-10;
    spatial.assign(static_cast<size_t>(KataSpatialC) * BoardArea, 0.0f);
    global.assign(static_cast<size_t>(KataGlobalC), 0.0f);

    const bool hasCurrentFirst = stage == 1 && lastloc.first != -1;
    const int committedStones = countStones(hasCurrentFirst);
    const array<double, BoardArea> priority = getPriorityValueArray(hasCurrentFirst);

    for(int y = 0; y < BoardH; y++) {
      for(int x = 0; x < BoardW; x++) {
        const int p = y * BoardW + x;
        spatial[p] = 1.0f; // on-board
        if(nextplayer == 0) {
          spatial[static_cast<size_t>(1) * BoardArea + p] = board[p];
          spatial[static_cast<size_t>(2) * BoardArea + p] = board[BoardArea + p];
        }
        else {
          spatial[static_cast<size_t>(1) * BoardArea + p] = board[BoardArea + p];
          spatial[static_cast<size_t>(2) * BoardArea + p] = board[p];
        }
      }
    }

    if(stage == 0) {
      if(committedStones == 0)
        global[1] = 1.0f;

      for(int y = 0; y < BoardH; y++) {
        for(int x = 0; x < BoardW; x++) {
          const int p = y * BoardW + x;
          const double pr = priority[static_cast<size_t>(p)];
          int count = 0;
          for(int yy = max(y - 1, 0); yy <= min(y + 1, BoardH - 1); yy++) {
            for(int xx = max(x - 1, 0); xx <= min(x + 1, BoardW - 1); xx++) {
              const int pp = yy * BoardW + xx;
              if(priority[static_cast<size_t>(pp)] - pr >= -PriorEps)
                count++;
            }
          }
          count--;
          for(int i = 0; i < count; i++)
            spatial[static_cast<size_t>(6 + i) * BoardArea + p] = 1.0f;
        }
      }
    }
    else {
      global[0] = 1.0f;
      if(committedStones == 0 || lastloc.first == -1)
        global[2] = 1.0f;
    }

    if(hasCurrentFirst) {
      const int p = lastloc.first * BoardW + lastloc.second;
      const double lastpr = priority[static_cast<size_t>(p)];
      spatial[static_cast<size_t>(3) * BoardArea + p] = 1.0f;
      spatial[static_cast<size_t>(1) * BoardArea + p] = 1.0f;

      for(int y = 0; y < BoardH; y++) {
        for(int x = 0; x < BoardW; x++) {
          if(!isLegal(x, y))
            continue;
          const int pp = y * BoardW + x;
          if(priority[static_cast<size_t>(pp)] + PriorEps >= lastpr)
            spatial[static_cast<size_t>(4) * BoardArea + pp] = 1.0f;
        }
      }
    }
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

  int countStones(bool excludeCurrentFirst) const {
    int total = 0;
    for(int y = 0; y < BoardH; y++) {
      for(int x = 0; x < BoardW; x++) {
        if(excludeCurrentFirst && y == lastloc.first && x == lastloc.second)
          continue;
        const int p = y * BoardW + x;
        if(board[static_cast<size_t>(p)] != 0.0f || board[static_cast<size_t>(BoardArea + p)] != 0.0f)
          total++;
      }
    }
    return total;
  }

  array<double, BoardArea> getPriorityValueArray(bool excludeCurrentFirst = false) const {
    array<double, BoardArea> distances{};
    double totalWeight = 0.0;
    double xWeighted = 0.0;
    double yWeighted = 0.0;
    for(int y = 0; y < BoardH; y++) {
      for(int x = 0; x < BoardW; x++) {
        if(excludeCurrentFirst && y == lastloc.first && x == lastloc.second)
          continue;
        const double stones = at(0, y, x) + at(1, y, x);
        totalWeight += stones;
        xWeighted += x * stones;
        yWeighted += y * stones;
      }
    }
    if(totalWeight <= 0.0f)
      return distances;

    const double xCentroid = xWeighted / totalWeight;
    const double yCentroid = yWeighted / totalWeight;
    for(int y = 0; y < BoardH; y++) {
      for(int x = 0; x < BoardW; x++) {
        const double dx = x - xCentroid;
        const double dy = y - yCentroid;
        distances[static_cast<size_t>(y * BoardW + x)] = dx * dx + dy * dy;
      }
    }
    return distances;
  }
};

static void loadRequiredModel(ConnectSixNet& c6Net, KataStyleNet& kataNet, bool& useKataNet) {
  // Alice must use this exact local model. Do not fall back to other models.
  const vector<string> paths = {
    "data/con6_6x96.bin"
  };

  string lastTried;
  string lastError;
  for(const string& path : paths) {
    lastTried = path;
    try {
      if(kataNet.load(path)) {
        if(kataNet.modelVersion != 102 || kataNet.spatialC != 22 || kataNet.globalC != 39
          || kataNet.blocks <= 0 || kataNet.channels <= 0)
          throw runtime_error("wrong kata-style model shape");
        useKataNet = true;
        return;
      }
    }
    catch(const exception& e) {
      lastError = path + ": " + e.what();
    }

    try {
      if(c6Net.load(path)) {
        useKataNet = false;
        return;
      }
    }
    catch(const exception& e) {
      lastError = path + ": " + e.what();
    }
  }

  if(lastError.empty())
    throw runtime_error("model not found: " + lastTried);
  throw runtime_error("model load failed: " + lastError);
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

static pair<int, int> modelMove(const KataStyleNet& net, const Board& board, int nextplayer, int movenum) {
  vector<float> spatial;
  vector<float> global;
  board.getKataInput(nextplayer, spatial, global);
  array<float, BoardArea> policy{};
  net.forwardPolicy(spatial, global, policy);

  float maxLogit = -numeric_limits<float>::infinity();
  for(int i = 0; i < BoardArea; i++) {
    const int x = i % BoardW;
    const int y = i / BoardW;
    if(board.isLegal(x, y))
      maxLogit = max(maxLogit, policy[static_cast<size_t>(i)]);
  }
  if(!isfinite(maxLogit))
    throw runtime_error("bad kata-style policy logits");

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
    throw runtime_error("bad kata-style policy distribution");

  static mt19937 rng(static_cast<unsigned int>(
    chrono::high_resolution_clock::now().time_since_epoch().count() ^ 0x9e3779b9U
  ));
  uniform_real_distribution<double> dist(0.0, total);
  double r = dist(rng);
  for(int i = 0; i < BoardArea; i++) {
    r -= weights[static_cast<size_t>(i)];
    if(r <= 0.0)
      return make_pair(i % BoardW, i / BoardW);
  }

  throw runtime_error("failed to sample kata-style policy");
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

  ConnectSixNet c6Net;
  KataStyleNet kataNet;
  bool useKataNet = false;
  loadRequiredModel(c6Net, kataNet, useKataNet);

  int outX[2] = {-1, -1};
  int outY[2] = {-1, -1};

  for(int stage = 0; stage < 2; stage++) {
    pair<int, int> loc = tacticalMove(board, nextPlayer);
    if(loc.first == -1) {
      if(useKataNet)
        loc = modelMove(kataNet, board, nextPlayer, static_cast<int>(requests.size()) * 2 + stage);
      else
        loc = modelMove(c6Net, board, nextPlayer, static_cast<int>(requests.size()) * 2 + stage);
    }

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
