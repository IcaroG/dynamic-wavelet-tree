import math

node_id = 0


def bitstr(x, nbits):
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
        self.absize = len(ab)
        nbits = int(math.ceil(math.log2(self.absize)))
        self.raw_codes = {ab[i]:bitstr(i, nbits) for i in range(self.absize)}
        self.ab = {}
        assert '#' not in ab
        self.root =  Node('#')
        self.zeronode = self.root
        self.sortednodes = [self.root]
    
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
            zeronode = self.zeronode #sortednodes[0]
            zeronode_par = zeronode.par
            par = Node()
            par.setchd(zeronode, 0)
            par.setchd(cnode, 1)
            if zeronode_par:
                zeronode_par.setchd(par, 0 if zeronode_par.chd[0] == zeronode else 1)
            else:
                self.root = par
            cur = cnode
            while cur.par:
                cur = cur.par
                cur.freq += 1
            self.sortednodes[1:1] = [cnode, par]
            n = len(self.sortednodes)
            for i in range(n-1):
                j = self.succ(i)
                if j != i:
                    assert j < n
                    assert self.sortednodes[j].freq < self.sortednodes[i].freq
                    assert j==n-1 or self.sortednodes[j+1].freq >= self.sortednodes[i].freq
                    node_i = self.sortednodes[i]
                    node_j = self.sortednodes[j]
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
                    self.sortednodes[i] = node_j
                    self.sortednodes[j] = node_i
        n = len(self.sortednodes)
        assert self.sortednodes[n-1].par == None
        for i in range(0,n-1,2):
            assert self.sortednodes[i].par == self.sortednodes[i+1].par
            assert self.sortednodes[i].freq <= self.sortednodes[i+1].freq
        
    def succ(self, i):
        l = i+1
        r = len(self.sortednodes)
        assert l < r   
        if self.sortednodes[r-1].freq < self.sortednodes[i].freq:
            return r-1
        if self.sortednodes[l].freq >= self.sortednodes[i].freq:
            return i
        # result in [l,r)
        while (r-l) > 1:
            m = (l + r) // 2
            if self.sortednodes[m].freq < self.sortednodes[i].freq:
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
        
    def printhuff(self):
        print("--------------------------------")
        self.print_tree(self.root)
        print("sorted_nodes:")
        for node in self.sortednodes:
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
        cur = self.zeronode 
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
        hc.printhuff()
    return hc,code
        
def decode(code, ab):
    n = len(code)
    i = 0
    hc = Huff(ab)
    abbits = int(math.ceil(math.log2(len(ab))))
    raw_codes = {bitstr(i, abbits):ab[i] for i in range(len(ab))}
    txt = ""
    cur = hc.root
    cw = ""
    while i < n or cur != hc.root:
        if i < n and code[i] == ' ':
            i += 1
            continue
        if cur.is_leaf():
            if cur == hc.zeronode:
                cw = code[i:i+abbits]
                assert cw in raw_codes
                c = raw_codes[cw]       
                txt += c
                i += abbits 
            else:
                c = cur.c 
                txt += c
                #i += 1
            hc.update(c)
            cw = ""
            cur = hc.root
        else:
            cur = cur.chd[int(code[i])]
            cw += code[i]
            i += 1 
    return txt       

txt = "aabbrracadabra"
ab = "abcdr"
hc, code = encode(txt, ab)
print("code =", code)

decoded = decode(code, ab)
print("decoded txt=",decoded)
assert decoded == txt


