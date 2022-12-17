import math

node_id = 0


def bin_str(x, nbits):
    ret = nbits * ['0']
    while nbits:
        nbits -= 1
        ret[nbits] = '1' if (x % 2) else '0'
        x >>= 1
    return "".join(ret) 

class Node:
    def __init__(self, c='\0', freq=0):
        global node_id
        self.id = node_id
        node_id += 1
        self.c = c
        self.freq = freq
        self.par = None
        self.chd = [None, None]
        
    def setchd(self, chd, pos):
        self.chd[pos] = chd
        chd.par = self      
        
    def is_leaf(self):
        return not (self.chd[0] or self.chd[1])

class Huff:
    def __init__(self, ab):
        self.ab_size = len(ab)
        nbits = int(math.ceil(math.log2(self.ab_size)))
        self.raw_codes = {ab[i]:bin_str(i, nbits) for i in range(self.ab_size)}
        self.ab = {}
        assert '#' not in ab
        self.root =  Node('#')
        self.null_node = self.root
        self.sorted_nodes = [self.root]
    
    def update(self, c):
        if c in self.ab:
            cur = self.ab[c]
            assert cur.is_leaf()
            cur.freq += 1
            while cur.par:
                cur = cur.par
                cur.freq += 1
        else:
            cnode = Node(c=c, freq=1)
            self.ab[c] = cnode
            null_node = self.null_node #sortednodes[0]
            null_node_par = null_node.par
            par = Node()
            par.setchd(null_node, 0)
            par.setchd(cnode, 1)
            if null_node_par:
                null_node_par.setchd(par, 0 if null_node_par.chd[0] == null_node else 1)
            else:
                self.root = par
            cur = cnode
            while cur.par:
                cur = cur.par
                cur.freq += 1
            self.sorted_nodes[1:1] = [cnode, par]
            n = len(self.sorted_nodes)
            for i in range(n-1):
                j = self.succ(i)
                if j != i:
                    assert j < n
                    assert self.sorted_nodes[j].freq < self.sorted_nodes[i].freq
                    assert j==n-1 or self.sorted_nodes[j+1].freq >= self.sorted_nodes[i].freq
                    node_i = self.sorted_nodes[i]
                    node_j = self.sorted_nodes[j]
                    par_i = node_i.par
                    pos_i = 0 if par_i.chd[0] == node_i else 1
                    assert par_i.chd[pos_i] == node_i
                    par_j = node_j.par
                    pos_j = 0 if par_j.chd[0] == node_j else 1
                    assert par_j.chd[pos_j] == node_j
                    par_i.setchd(node_j, pos_i)
                    par_j.setchd(node_i, pos_j)
                    for cur in [node_i, node_j]:
                        while cur.par:
                            cur = cur.par
                            cur.freq = cur.chd[0].freq + cur.chd[1].freq
                    self.sorted_nodes[i] = node_j
                    self.sorted_nodes[j] = node_i
        n = len(self.sorted_nodes)
        assert self.sorted_nodes[n-1].par == None
        for i in range(0,n-1,2):
            assert self.sorted_nodes[i].par == self.sorted_nodes[i+1].par
            assert self.sorted_nodes[i].freq <= self.sorted_nodes[i+1].freq
        
    def succ(self, i):
        l = i+1
        r = len(self.sorted_nodes)
        assert l < r   
        if self.sorted_nodes[r-1].freq < self.sorted_nodes[i].freq:
            return r-1
        if self.sorted_nodes[l].freq >= self.sorted_nodes[i].freq:
            return i
        # result in [l,r)
        while (r-l) > 1:
            m = (l + r) // 2
            if self.sorted_nodes[m].freq < self.sorted_nodes[i].freq:
                l = m
            else:
                r = m 
        return l

    def print_tree(self, root, level=0):
        if not root:
            return
        print(level*"  ", end='')
        print("[id=%d freq=%d char=%c]"%(root.id, root.freq, root.c))
        self.print_tree(root.chd[0], level+1)
        self.print_tree(root.chd[1], level+1)
        
    def print_huff(self):
        print("--------------------------------")
        self.print_tree(self.root)
        print("sorted_nodes:")
        for node in self.sorted_nodes:
            print("id=%d freq=%d"%(node.id, node.freq))
        print("--------------------------------")

    def has_char(self, c):
        return c in self.ab
        
    def char_code(self, c):
        if c not in self.ab:
            return self.raw_codes[c]
        cur = self.ab[c]
        code = []
        while cur.par:
            par = cur.par
            code.append('0' if par.chd[0] == cur else '1')
            cur = par
        return "".join(code[::-1])
    
    def null_code(self):
        cur = self.null_node 
        code = []
        while cur.par:
            par = cur.par
            code.append('0' if par.chd[0] == cur else '1')
            cur = par
        return "".join(code[::-1])
         

def encode(txt, ab):
    n = len(txt)
    hc = Huff(ab)
    code = ""
    for i in range(n):
        print("addind txt[%d]=%c"%(i, txt[i]))
        if hc.has_char(txt[i]):
            code = code + hc.char_code(txt[i]) + " "
        else: 
            code = code + hc.null_code() + hc.char_code(txt[i]) + " "
        hc.update(txt[i])
        hc.print_huff()
    return hc,code
        
def decode(code, ab):
    n = len(code)
    i = 0
    hc = Huff(ab)
    ab_nbits = int(math.ceil(math.log2(len(ab))))
    raw_codes = {bin_str(i, ab_nbits):ab[i] for i in range(len(ab))}
    txt = ""
    cur = hc.root
    codeword = ""
    while i < n or cur != hc.root:
        if i < n and code[i] == ' ':
            i += 1
            continue
        if cur.is_leaf():
            if cur == hc.null_node:
                codeword = code[i : i + ab_nbits] 
                assert codeword in raw_codes
                c = raw_codes[codeword]       
                txt += c
                i += ab_nbits 
            else:
                c = cur.c 
                txt += c
                #i += 1
            hc.update(c)
            codeword = ""
            cur = hc.root
        else:
            cur = cur.chd[int(code[i])]
            codeword += code[i]
            i += 1 
    return txt       

txt = "aabbrracadabra"
ab = "abcdr"
hc, code = encode(txt, ab)
print("code =", code)

decoded = decode(code, ab)
print("decoded txt=",decoded)
assert decoded == txt


