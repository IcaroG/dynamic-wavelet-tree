#include <bits/stdc++.h>

using namespace std;

int main() {
  string s;
  int n;
  cin >> s >> n;
  ifstream fs{s};
  string txt;
  copy_n(istreambuf_iterator<char>(fs), n, back_inserter(txt));
  ofstream out{s + to_string(n)};
  out << txt;
  out.close();
  return 0;
}