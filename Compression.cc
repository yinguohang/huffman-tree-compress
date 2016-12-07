#include <iostream>
#include <stdio.h>
#include <string>
#include <fstream>
#include <cstdlib>
#include <map>
#include <vector>
using namespace std;

#define NUMBER_OF_CHAR 256

struct TreeNode{
  char c;
  TreeNode* left;
  TreeNode* right; 
  TreeNode():c(0),left(0),right(0){}
  ~TreeNode(){
    if (left != 0){
      delete left;
      delete right;
    }
  }
};

class reader{
  char buffer[8];
  int ptr;
  ifstream* ist;
  bool ended;
public:
  ~reader(){
    ist->close();
    delete ist;
  }
  reader(string s){
    for (int i = 0; i < 8; i++) buffer[i] = 0;
    ptr = 0;
    ist = new ifstream(s.c_str(),ios::in | ios::binary);
    read_from_file();
    ended = 0;
  }
  void read_from_file(){
    char c;
    if (ist->get(c)) {
      unsigned char cc = c;
      int temp = 128;
      for (int i = 0; i < 8; i++){
        buffer[i] = cc / temp;
        cc = cc % temp;
        temp = temp >> 1;    
      }
      ptr = 0;
      return;
    }
    ended = 1;
    ptr = 0;
  }
  int read_as_number(int byte_num){
    int rtn = 0;
    for (int i = 0; i < byte_num; i++){
      if (ptr == 8) read_from_file();
      rtn = (rtn << 1) + buffer[ptr];
      ptr++;
    }
    return rtn;
  }
  int place() { return ptr; }
  bool is_ended() { return ended; }
};

class writer{
  char buffer[8];
  int ptr;
  ofstream* ost;
  bool ended;
public:  
  writer(string s){
    for (int i = 0; i < 8; i++) buffer[i] = 0;
    ptr = 0;
    ost = new ofstream(s.c_str(),ios::out | ios::binary);
  }
  void write(vector<char>& v){
    int len = v.size();
    for (int i = 0; i < len; i++){
      write(v[i]);
    }
  }
  void write(char c){
    buffer[ptr] = c;
    ptr++;
    if (ptr == 8){
      char temp = 0;
      for (int i = 0; i < 8; i++){
        temp = (temp << 1) + buffer[i];
      }
      ost->write(&temp, 1);
      ptr = 0;
    }
  }
  void write_char(char c){
    unsigned char cc = c;
    int temp = 128;
    for (int i = 0; i < 8; i++){
      write(cc / temp);
      cc = cc % temp;
      temp = temp >> 1;
    }  
  }
  void finish(){
    if (ptr == 0) return;
    char temp = 0;
    for (int i = 0; i < ptr; i++)
      temp = (temp << 1) + buffer[i];
    for (int i = ptr; i < 8; i++)
      temp = temp >> 1;
    ost->write(&temp, 1);
    ost->close();
  }
  int place() { return ptr; }
  ~writer() { delete ost; }
};

void record_tree(TreeNode* now, writer& w, map<char, vector<char> >& code){
  if (now -> left == 0 && now -> right == 0){
    w.write(0); 
    w.write_char(now -> c);
  }else{
    w.write(1);
    record_tree(now -> left, w, code);
    record_tree(now -> right, w, code); 
  }
}

void get_code(TreeNode* now, map<char, vector<char> >& code, vector<char>& v){
  if (now -> left == 0 && now -> right == 0){
    code[now -> c] = v;
    return;
  }
  v.push_back(0);
  get_code(now -> left, code, v);
  v[v.size() - 1] = 1;
  get_code(now -> right, code, v);
  v.pop_back();
}

void compress(string inputFilename, string outputFilename) {
  writer w(outputFilename);
  ifstream ist(inputFilename.c_str(), ios::in | ios::binary);
  int times[NUMBER_OF_CHAR];
  //Initialize.
  for (int i = 0; i < NUMBER_OF_CHAR; i++) times[i] = 0;
  char c;
  //Sum up the number.
  while (ist.get(c))
    times[(int)(unsigned char)(c)]++;
  //Build the Huffman Tree
  multimap<int, TreeNode*> forest;
  for (int i = 0; i < NUMBER_OF_CHAR; i++){
    if (times[i] == 0) continue;
    TreeNode* t = new TreeNode();
    t -> c = i;
    forest.insert(make_pair(times[i], t));
  }
  while (forest.size() >= 2){
    multimap<int, TreeNode*>::iterator iter1 = forest.begin();
    multimap<int, TreeNode*>::iterator iter2 = forest.begin();
    iter2++;
    TreeNode* t = new TreeNode();
    int num = iter1 -> first + iter2 -> first;
    t -> left = iter1 -> second;
    t -> right = iter2 -> second;
    forest.erase(iter1);
    forest.erase(iter2);
    forest.insert(make_pair(num, t));
  }
  map<char, vector<char> > code;
  //Exception: Only 1 char or No char.
  TreeNode* now = forest.begin() -> second;
  vector<char> v ;
  //Write Tree
  //Plans: (1) Write Down the times
  //       (2) Write Down the Tree      Selected!
  //       (3) Write Down the Encoding
  if (forest.size() != 0){
    w.write(1);
    get_code(now, code, v);
    now = forest.begin() -> second;
    record_tree(now, w, code);
  }else{
    w.write(0);
    w.finish();
    return;
  }
  //Write Padding: Make sure that bits'number can be divided by 8.
  int pad = (w.place() + 3) % 8;
  for (int i = 0; i < NUMBER_OF_CHAR; i++){
    if (times[i] == 0) continue;
    pad = (pad + times[i] * code[i].size()) % 8;
  }
  int p = pad;
  w.write(pad / 4);
  pad = pad % 4;
  w.write(pad / 2);
  pad = pad % 2;
  w.write(pad);
  if (p != 0){
    for (int i = p; i < 8; i++)
      w.write(0);
  }
  //Write Data
  ist.clear();
  ist.seekg(0, ios::beg);
  while (ist.get(c)){
    w.write(code[c]);
  }
  ist.close();
  delete forest.begin() -> second;
};

TreeNode* read_tree(reader& r){
  TreeNode* now = new TreeNode();
  if (r.read_as_number(1) == 1){
    now -> left = read_tree(r);
    now -> right = read_tree(r);
    return now;
  }else{
    now -> c = r.read_as_number(8);
    return now;
  }
}

char read_number(reader& r, TreeNode* root){
  int now = r.read_as_number(1);
  TreeNode* ptr = root;
  if (r.is_ended()) return 0;
  if (ptr -> left != 0){
    if (now == 0)
      ptr = ptr -> left;
    else
      ptr = ptr -> right;
  }
  while (ptr -> left != 0){
    if (r.read_as_number(1)){
      ptr = ptr -> right;
    }else{
      ptr = ptr -> left;
    }
  }
  return ptr -> c;
}

void decompress(string inputFilename, string outputFilename) {
  reader r(inputFilename);
  ofstream ost(outputFilename.c_str(),ios::out | ios::binary);
  //1. Whether it is empty.
  if (r.read_as_number(1) != 1){
    ost.close();
    return;
  }
  //2. Read the tree.
  TreeNode* root = read_tree(r); 
  //3. Read the padding.
  int pad = r.read_as_number(3);
  if (pad != 0){
    r.read_as_number(8 - pad);
  }
  //4. Recovery
  while (1){
    char c = read_number(r, root);
    if (r.is_ended()){
      ost.close();
      break;  
    }
    ost.write(&c, 1);
  }
  delete root;
}

void usage(string prog) {
  cerr << "Usage: " << endl << "\t" << prog << " [-d] input_file output_file" << endl;
  exit(2);
}

int main(int argc, char* argv[]) {
  int i;
  string inputFilename, outputFilename;
  bool isDecompress = false;
  for (i = 1; i < argc; i++) {
    if (argv[i] == string("-d")) isDecompress = true;
    else if (inputFilename == "") inputFilename = argv[i];
    else if (outputFilename == "") outputFilename = argv[i];
    else usage(argv[0]);
  }
  if (outputFilename == "") usage(argv[0]);
  if (isDecompress) 
    decompress(inputFilename, outputFilename);
  else
    compress(inputFilename, outputFilename);
  return 0;
}
