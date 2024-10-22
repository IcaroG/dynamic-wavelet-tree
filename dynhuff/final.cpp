#include <bits/stdc++.h>

#include <bit_vector/bv.hpp>
#include <filesystem>

using namespace std;

const char NULL_CHAR = 127;

int swaps = 0;
long long totalChangedBits = 0;
int nodeId = 0;
chrono::duration<double, milli> worstTime;

vector<bool> binCode(int x, int nBits) {
  vector<bool> ret(nBits, 0);
  while (nBits) {
    nBits--;
    ret[nBits] = x % 2 ? 1 : 0;
    x >>= 1;
  }
  return ret;
}

string mapToString(const unordered_map<char, bool> &mp) {
  const string delimiter = ",";
  return "";
  // return "{" +
  //        accumulate(
  //            mp.begin(), mp.end(), string(),
  //            [delimiter](const string &s, const pair<const char, int> &p) {
  //              return s + (s.empty() ? string() : delimiter) + p.first + ": " +
  //                     to_string(p.second);
  //            }) +
  //        "}";
}

int offset = 0;

class TNode {
 public:
  int id, freq;
  int revindex;
  char c;
  TNode *par, *chd[2];
  unordered_map<char, bool> alph;
  bv::small_bv<32, 38400, 16> bit;

  TNode(char _c = '\0', int _freq = 0, int revid = 0) {
    id = nodeId++;
    c = _c;
    freq = _freq;
    par = NULL;
    chd[0] = chd[1] = NULL;
    if (_c != '\0') alph.emplace(c, -1);
    revindex = revid - offset;
  }

  void setChild(TNode *newChd, int pos) {
    chd[pos] = newChd;
    newChd->par = this;
  }

  bool isLeaf() { return !chd[0] && !chd[1]; }
};

class DynamicWaveletHuff {
 public:
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
    int st = nodeUpdate->revindex + offset;
    //cout << st << endl;
    for (int i = st, j = st; i < n && nodeUpdate != NULL;) {
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
      if(nodeUpdate != NULL) {
        i = nodeUpdate->revindex + offset;
      }
    }
  }

  void printHuff() {
    cout << "------------------------" << endl;
    printTree(root);
    cout << "------------------------" << endl;
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

 private:
  int abSize, nBits;
  unordered_map<char, TNode *> ab;
  unordered_map<char, vector<bool>> rawCodes;
  TNode *root, *nullNode;
  deque<TNode *> sortedNodes;

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
    offset += 2;
    TNode *newNode = new TNode(c, 1, 1);
    ab[c] = newNode;
    TNode *nullNodeParent = nullNode->par;
    TNode *newPar = new TNode('\0', 0, 2);
    nullNode->revindex = -offset;
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
    swap(sortedNodes[i]->revindex, sortedNodes[j]->revindex);
    swap(sortedNodes[i], sortedNodes[j]);
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

    fixDown(pathA, lca, changesB, b->alph, a->alph);
    fixDown(pathB, lca, changesA, a->alph, b->alph);
  }

  tuple<vector<bool>, vector<int>, bool> getLcaChanges(TNode *a, TNode *lca) {
    TNode *cur = a->par;
    TNode *lst = a;
    bool fromIdx = cur->chd[0] == a ? 0 : 1;
    vector<bool> path = {fromIdx};
    int numberOfChanges = a->bit.size() == 0 ? a->freq : a->bit.size();
    vector<int> pos(numberOfChanges, 0);
    vector<int> newPos(numberOfChanges, 0);
    totalChangedBits += pos.size();
    for (int i = 0; i < (int)pos.size(); ++i) {
      pos[i] = i;
    }
    while (cur != lca) {
      fastMultiSelect(cur, pos, newPos, fromIdx);
      totalChangedBits += pos.size();
      for (int i = pos.size() - 1; i >= 0; --i) {
        cur->bit.remove(newPos[i]);
      }
      lst = cur;
      cur = cur->par;
      fromIdx = cur->chd[0] == lst ? 0 : 1;
      path.push_back(fromIdx);
      pos.swap(newPos);
    }
    
    fastMultiSelect(lca, pos, newPos, fromIdx);
    return {path, newPos, fromIdx ^ 1};
  }

  void fastMultiSelect(TNode *cur, vector<int> &pos, vector<int> &newPos, bool v) {
    if(pos.size() * 32 >= cur->bit.size()) {
      int c = 0,  k = 0;
      for(int i = 0; i < (int)cur->bit.size(); ++i) {
        if(cur->bit.at(i) == v) {
          if(c == pos[k]) {
            newPos[k] = i;
            k++;
          }
          c++;
        }
      }
    } else {
      for (int i = 0; i < (int)pos.size(); ++i) {
        newPos[i] = cur->bit.select(v, pos[i] + 1);
      }
    }
  }

  vector<pair<int, int>> getChanges(TNode *lca, const vector<int> &pos,
                                    bool bit) {
    vector<pair<int, int>> changes;
    int nxtPos = -1;
    totalChangedBits += pos.size();
    for (int i = 0; i < (int)pos.size(); ++i) {
      if(nxtPos == -1) {
        nxtPos = lca->bit.rank(bit, pos[i]);
      } else if(pos[i] != pos[i - 1] + 1) {
          nxtPos = lca->bit.rank(bit, pos[i]);
      }
      if (changes.empty() || changes.back().first != nxtPos)
        changes.emplace_back(nxtPos, 1);
      else
        changes.back().second++;
    }
    
    return changes;
  }

  void fixDown(const vector<bool> &path, TNode *node,
               vector<pair<int, int>> &changes,
               const unordered_map<char, bool> &keep,
               const unordered_map<char, bool> &rem) {
    TNode *cur = node;
    vector<pair<int, int>> nxtChanges;
    for (int i = path.size() - 1; i >= 0; --i) {
      bool changedTo = path[i];
      if (i != (int)path.size() - 1) {
        for (auto [v, c] : rem) {
          cur->alph.erase(v);
        }
      }
      for (auto [v, c] : keep) {
        cur->alph[v] = changedTo;
      }
      if (i == 0) break;
      cur = cur->chd[path[i]];
      bool nxtBit = path[i - 1];
      for (auto [v, c] : changes) {
        int nxtPos = cur->bit.rank(nxtBit, v);
        while (c--) {
          if (nxtChanges.empty() || nxtChanges.back().first != nxtPos)
            nxtChanges.emplace_back(nxtPos, 1);
          else
            nxtChanges.back().second++;
          cur->bit.insert(v, nxtBit);
        }
      }
      nxtChanges.swap(changes);
      nxtChanges.clear();
    }
  }

  void printTree(TNode *cur, int level = 0) {
    if (!cur) {
      return;
    }
    string spaces(level * 2, ' ');
    string bitVecStr;
    for (int i = 0; i < (int)cur->bit.size(); ++i) {
      bitVecStr += to_string(cur->bit.at(i));
    }
    cout << spaces;
    printf("[id=%d freq=%d char=%c alph=%s]\n", cur->id, cur->freq,
           cur->c, mapToString(cur->alph).c_str());
    printTree(cur->chd[0], level + 1);
    printTree(cur->chd[1], level + 1);
  }

  void assertWaveletNode(TNode *cur, string txt) {
    if (cur->isLeaf()) return;
    string s;
    for (int i = 0; i < (int)cur->bit.size(); ++i) {
      s += to_string(cur->bit.at(i));
    }
    if (cur->bit.size() != txt.size()) {
      printf("id = %d with wrong %s %s", cur->id, s.c_str(), txt.c_str());
      cout << endl;
      assert(false);
    }
    for (int i = 0; auto c : txt) {
      if (cur->alph[c] != s[i] - '0') {
        printf("id = %d %d with wrong %s %s", cur->id, i, s.c_str(), txt.c_str());

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
    cout << "adding " << c << endl;
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
  vector<string> paths;
  for (auto testFile : filesystem::directory_iterator("./tests")) {
    paths.push_back(testFile.path().string());
  }
  sort(paths.begin(), paths.end());
  for (auto testFile : paths) {
    ifstream fs{testFile};
    string txt((istreambuf_iterator<char>(fs)), istreambuf_iterator<char>());
    set<char> ab;
    for (auto c : txt) ab.insert(c);
    DynamicWaveletHuff wv(ab);
    swaps = 0;
    totalChangedBits = 0;
    auto start = chrono::high_resolution_clock::now();
    for (auto c : txt) {
      wv.update(c);
    }
    auto end = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> timeMs = end - start;

    cout << fixed << setprecision(5) << testFile << " took " << timeMs.count()
         << "ms"
         << " with " << swaps << " swaps and " << totalChangedBits
         << " changed bits and alph size equal to " << ab.size();
    cout << endl;
    wv.assertWavelet(txt);
  }
  return 0;
}