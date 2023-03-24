#include <bits/stdc++.h>

#include <dynamic/dynamic.hpp>
#include <filesystem>

using namespace std;

const char NULL_CHAR = 128;

int swaps = 0;
int totalChangedBits = 0;
int nodeId = 0;

vector<bool> binCode(int x, int nBits) {
  vector<bool> ret(nBits, 0);
  while (nBits) {
    nBits--;
    ret[nBits] = x % 2 ? 1 : 0;
    x >>= 1;
  }
  return ret;
}

string mapToString(const unordered_map<char, int> &mp) {
  const string delimiter = ",";
  return "{" +
         accumulate(
             mp.begin(), mp.end(), string(),
             [delimiter](const string &s, const pair<const char, int> &p) {
               return s + (s.empty() ? string() : delimiter) + p.first + ": " +
                      to_string(p.second);
             }) +
         "}";
}

struct TNode {
  int id, freq;
  char c;
  TNode *par, *chd[2];
  unordered_map<char, int> alph;
  dyn::suc_bv bit;

  TNode(char _c = '\0', int _freq = 0) {
    id = nodeId++;
    c = _c;
    freq = _freq;
    par = NULL;
    chd[0] = chd[1] = NULL;
    if (_c != '\0') alph.emplace(c, -1);
  }

  void setChild(TNode *newChd, int pos) {
    chd[pos] = newChd;
    newChd->par = this;
  }

  bool isLeaf() { return !chd[0] && !chd[1]; }
};

struct DynamicWaveletHuff {
  int abSize, nBits;
  unordered_map<char, TNode *> ab;
  unordered_map<char, vector<bool>> rawCodes;
  TNode *root, *nullNode;
  deque<TNode *> sortedNodes;

  DynamicWaveletHuff(const set<char> &_ab) {
    abSize = _ab.size();
    nBits = 32 - __builtin_clz(abSize - 1);
    assert(!_ab.count(NULL_CHAR));
    for (int idx = 0; auto c : _ab) {
      rawCodes[c] = binCode(idx, nBits);
      idx++;
    }
    root = new TNode(NULL_CHAR);
    nullNode = root;
    sortedNodes.push_back(root);
  }

  void update(char c) {
    addBit(c);
    TNode *nodeUpdate = updateLeaf(c);
    int n = sortedNodes.size();
    for (int i = 0, j = 0; i < n && nodeUpdate != NULL;) {
      if (sortedNodes[i] != nodeUpdate) {
        i++;
        continue;
      }
      j = i + 1;
      while (j < n && sortedNodes[j]->freq == sortedNodes[i]->freq - 1) j++;
      j -= 1;
      if (sortedNodes[j] == sortedNodes[i]->par) j--;
      if (i < j && sortedNodes[j]->freq == sortedNodes[i]->freq - 1) {
        nodeUpdate = sortedNodes[j]->par;
        swapNodes(i, j);
      } else {
        if (nodeUpdate->par != NULL) {
          nodeUpdate->par->freq = nodeUpdate->par->freq + 1;
        }
        nodeUpdate = nodeUpdate->par;
      }
      i = j + 1;
    }
  }

  void addBit(char c) {
    char goalChar = c;
    if (!ab.count(c)) {
      goalChar = NULL_CHAR;
    }
    TNode *cur = root;
    while (cur->c != goalChar) {
      int nextChild = cur->alph[goalChar];
      if (!cur->alph.count(c)) {
        cur->alph[c] = nextChild;
      }
      cur->bit.insert(cur->bit.size(), nextChild);
      cur = cur->chd[nextChild];
    }
  }

  TNode *updateLeaf(char c) {
    if (ab.count(c)) {
      TNode *cur = ab[c];
      assert(cur->isLeaf());
      cur->freq++;
      return cur;
    }
    TNode *newNode = new TNode(c, 1);
    ab[c] = newNode;
    TNode *nullNodeParent = nullNode->par;
    TNode *newPar = new TNode();
    newPar->setChild(nullNode, 0);
    newPar->setChild(newNode, 1);
    newPar->alph.emplace(NULL_CHAR, 0);
    newPar->alph.emplace(c, 1);
    newPar->bit.insert(0, 1);
    if (nullNodeParent != NULL) {
      nullNodeParent->setChild(newPar,
                               nullNodeParent->chd[0] == nullNode ? 0 : 1);
    } else {
      root = newPar;
    }
    sortedNodes.pop_front();
    sortedNodes.push_front(newPar);
    sortedNodes.push_front(newNode);
    sortedNodes.push_front(nullNode);
    return newNode;
  }

  void swapNodes(int i, int j) {
    swaps++;
    TNode *u = sortedNodes[i], *v = sortedNodes[j];
    TNode *anc = lca(u, v);
    fixBits(u, v, anc);
    TNode *parentU = u->par;
    int posU = parentU->chd[0] == u ? 0 : 1;
    TNode *parentV = v->par;
    int posV = parentV->chd[0] == v ? 0 : 1;
    parentU->setChild(v, posU);
    parentV->setChild(u, posV);
    parentV->freq += 1;
    swap(sortedNodes[i], sortedNodes[j]);
    fixAlph(u, anc, v->alph);
    fixAlph(v, anc, u->alph);
  }

  TNode *lca(TNode *u, TNode *v) {
    u->freq -= 1;
    TNode *curU = u, *curV = v;
    while (curU != curV) {
      while (curV != curU && curV->freq <= curU->freq) curV = curV->par;
      while (curU->freq < curV->freq) curU = curU->par;
    }
    u->freq += 1;
    return curU;
  }

  void fixBits(TNode *a, TNode *b, TNode *lca) {
    auto [pathA, posA, bitA] = getLcaChanges(a, lca);
    auto [pathB, posB, bitB] = getLcaChanges(b, lca);
    for (int i = 0; i < (int)max(posA.size(), posB.size()); ++i) {
      if (i < (int)posA.size()) lca->bit.set(posA[i], bitA);
      if (i < (int)posB.size()) lca->bit.set(posB[i], bitB);
    }
    auto changesA = getChanges(lca, posA, bitA);
    auto changesB = getChanges(lca, posB, bitB);
    fixDown(pathA, lca, changesB);
    fixDown(pathB, lca, changesA);
  }

  tuple<vector<bool>, vector<int>, bool> getLcaChanges(TNode *a, TNode *lca) {
    TNode *cur = a->par;
    TNode *lst = a;
    bool fromIdx = cur->chd[0] == a ? 0 : 1;
    vector<bool> path = {fromIdx};
    int numberOfChanges = a->bit.size() == 0 ? a->freq : a->bit.size();
    totalChangedBits += numberOfChanges;
    vector<int> pos(numberOfChanges, 0);
    vector<int> newPos(numberOfChanges, 0);
    for (int i = 0; i < (int)pos.size(); ++i) {
      pos[i] = i;
    }
    while (cur != lca) {
      for (int i = 0; i < (int)pos.size(); ++i) {
        newPos[i] = cur->bit.select(pos[i], fromIdx);
      }
      for (int i = pos.size() - 1; i >= 0; --i) {
        cur->bit.remove(newPos[i]);
      }
      lst = cur;
      cur = cur->par;
      fromIdx = cur->chd[0] == lst ? 0 : 1;
      path.push_back(fromIdx);
      pos.swap(newPos);
    }
    for (int i = 0; i < (int)pos.size(); ++i) {
      newPos[i] = lca->bit.select(pos[i], fromIdx);
    }
    return {path, newPos, fromIdx ^ 1};
  }

  vector<pair<int, int>> getChanges(TNode *lca, const vector<int> &pos,
                                    bool bit) {
    vector<pair<int, int>> changes;
    for (int i = 0; i < (int)pos.size(); ++i) {
      int nxtPos = lca->bit.rank(pos[i], bit);
      if (changes.empty() || changes.back().first != nxtPos)
        changes.emplace_back(nxtPos, 1);
      else
        changes.back().second++;
    }
    return changes;
  }

  void fixDown(const vector<bool> &path, TNode *node,
               vector<pair<int, int>> &changes) {
    TNode *cur = node;
    vector<pair<int, int>> nxtChanges;
    for (int i = path.size() - 1; i > 0; --i) {
      cur = cur->chd[path[i]];
      bool nxtBit = path[i - 1];
      int count = 0;
      for (auto [v, c] : changes) {
        while (c--) {
          int nxtPos = cur->bit.rank(v, nxtBit);
          if (nxtChanges.empty() || nxtChanges.back().first != nxtPos)
            nxtChanges.emplace_back(nxtPos, 1);
          else
            nxtChanges.back().second++;
          cur->bit.insert(v, nxtBit);
          count++;
        }
      }
      nxtChanges.swap(changes);
      nxtChanges.clear();
    }
  }

  void fixAlph(TNode *node, TNode *lca, unordered_map<char, int> &rem) {
    TNode *cur = node->par;
    bool fromIdx = cur->chd[0] == node ? 0 : 1;
    while (cur != lca) {
      for (auto [c, v] : rem) {
        cur->alph.erase(c);
      }
      for (auto [c, v] : node->alph) {
        cur->alph[c] = fromIdx;
      }
      fromIdx = cur->par->chd[0] == cur ? 0 : 1;
      cur = cur->par;
    }
    for (auto [c, v] : node->alph) {
      lca->alph[c] = fromIdx;
    }
  }

  void printTree(TNode *cur, int level = 0) {
    if (!cur) {
      return;
    }
    string spaces(level * 2, ' ');
    string bitVecStr;
    for (int i = 0; i < (int)cur->bit.size(); ++i) {
      bitVecStr += to_string(cur->bit[i]);
    }
    cout << spaces;
    printf("[id=%d freq=%d char=%c alph=%s bin=%s]\n", cur->id, cur->freq,
           cur->c, mapToString(cur->alph).c_str(), bitVecStr.c_str());
    printTree(cur->chd[0], level + 1);
    printTree(cur->chd[1], level + 1);
  }

  void printHuff() {
    cout << "------------------------" << endl;
    printTree(root);
    cout << "------------------------" << endl;
  }

  void assertWaveletNode(TNode *cur, string txt) {
    if (cur->isLeaf()) return;
    string s;
    for (int i = 0; i < (int)cur->bit.size(); ++i) {
      s += to_string(cur->bit[i]);
    }
    if (cur->bit.size() != txt.size()) {
      printf("id = %d with wrong %s %s", cur->id, s.c_str(), txt.c_str());
      cout << endl;
      assert(false);
    }
    for (int i = 0; auto c : txt) {
      if (cur->alph[c] != s[i] - '0') {
        printf("id = %d with wrong %s %s", cur->id, s.c_str(), txt.c_str());

        cout << endl;
        assert(false);
      }
      i++;
    }
    string nextTxt[2];
    for (int i = 0; i < (int)txt.size(); ++i) {
      nextTxt[cur->alph[txt[i]]] += txt[i];
    }
    for (int i = 0; i < 2; ++i) {
      if (cur->chd[i] != NULL) assertWaveletNode(cur->chd[i], nextTxt[i]);
    }
  }

  void assertWavelet(string &txt) { assertWaveletNode(root, txt); }

  bool hasChar(const char c) { return ab.count(c); }

  vector<bool> charCode(const char c) {
    if (!ab.count(c)) {
      return rawCodes[c];
    }
    return getCode(ab[c]);
  }

  vector<bool> nullCode() { return getCode(nullNode); }

  vector<bool> getCode(TNode *start) {
    TNode *cur = start;
    vector<bool> code;
    while (cur->par) {
      TNode *par = cur->par;
      code.push_back(par->chd[0] == cur ? 0 : 1);
      cur = par;
    }
    reverse(code.begin(), code.end());
    return code;
  }
};

pair<DynamicWaveletHuff, vector<vector<bool>>> encode(string t) {
  set<char> ab;
  for (auto c : t) ab.insert(c);
  DynamicWaveletHuff wv(ab);
  vector<vector<bool>> code;
  for (auto c : t) {
    if (wv.hasChar(c)) {
      code.push_back(wv.charCode(c));
    } else {
      auto nullCode = wv.nullCode();
      auto charCode = wv.charCode(c);
      nullCode.insert(nullCode.end(), charCode.begin(), charCode.end());
      code.push_back(nullCode);
    }
    wv.update(c);
    wv.printHuff();
  }

  return {wv, code};
}

int main() {
  for (auto testFile : filesystem::directory_iterator("./tests")) {
    ifstream fs{testFile.path()};
    string txt((istreambuf_iterator<char>(fs)), istreambuf_iterator<char>());
    set<char> ab;
    for (auto c : txt) ab.insert(c);
    DynamicWaveletHuff wv(ab);
    swaps = 0;
    totalChangedBits = 0;
    cout << txt.size() << ' ' << ab.size() << endl;
    auto start = chrono::high_resolution_clock::now();
    for (auto c : txt) {
      wv.update(c);
    }
    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> timeMs = end - start;

    cout << fixed << setprecision(9) << testFile.path() << " took "
         << timeMs.count() << "ms"
         << " with " << swaps << " swaps and " << totalChangedBits
         << " changed bits" << endl;
    wv.assertWavelet(txt);
  }
  return 0;
}