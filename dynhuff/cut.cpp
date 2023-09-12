#include <bits/stdc++.h>

using namespace std;

int main() {
  string s;
  int n;
  cin >> s >> n;
  ifstream fs{s};
  string txt;
  copy_n(istreambuf_iterator<char>(fs), n, back_inserter(txt));
  string aux;
  for(auto c : txt) {
    if(c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z')
      aux.push_back(c);
  }
  cout << aux.size() << endl;
  ofstream out{s + to_string(n)};
  out << aux;
  out.close();
  return 0;
}