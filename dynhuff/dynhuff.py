import math
import string
from typing import Optional
from bitstring import BitArray

node_id = 0
NULL_CHAR = '#'
bit_to_str = {0:'0b0', 1: '0b1', '0': '0b0', '1': '0b1'}
MAX_DISTANCE_LCA = 0
input_txt = "should be working for now"

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
        self.par: Optional[Node] = None
        self.chd: list[Optional[Node]] = [None, None]
        self.alph = {} if c == '\0' else {c: -1}
        self.bit: Optional[BitArray] = None
        
    def set_chd(self, chd, pos):
        self.chd[pos] = chd
        chd.par = self      
        
    def is_leaf(self):
        return not (self.chd[0] or self.chd[1])

class Huff:
    def __init__(self, ab):
        self.ab_size = len(ab)
        self.sz = 0
        nbits = int(math.ceil(math.log2(self.ab_size)))
        self.raw_codes = {ab[i]:bin_str(i, nbits) for i in range(self.ab_size)}
        self.ab = {}
        assert NULL_CHAR not in ab
        self.root =  Node(NULL_CHAR)
        self.null_node = self.root
        self.sorted_nodes = [self.root]
    
    def update(self, c):
        self.sz += 1
        self.add_bit(c)
        node_update = self.update_leaf(c)
        n = len(self.sorted_nodes)
        i = 0
        j = 0
        # Two pointers to find which nodes to swap
        while i < n and node_update != None:
            if self.sorted_nodes[i] != node_update:
                i += 1
                continue
            j = i + 1
            while j < n and self.sorted_nodes[j].freq == self.sorted_nodes[i].freq - 1:
                j += 1
            j -= 1
            if self.sorted_nodes[j] == self.sorted_nodes[i].par:
                j -= 1
            if i < j and self.sorted_nodes[j].freq == self.sorted_nodes[i].freq - 1:
                node_update = self.sorted_nodes[j].par
                self.swap_nodes(i, j)
            else:
                if node_update.par != None:
                    node_update.par.freq = node_update.par.freq + 1
                node_update = node_update.par
            i = j + 1
        n = len(self.sorted_nodes)
        assert self.sorted_nodes[n-1].par == None
        self.assert_wavelet(input_txt[:self.sz])
        for i in range(0,n-1):
            assert self.sorted_nodes[i].par == self.sorted_nodes[i^1].par
            assert self.sorted_nodes[i].freq <= self.sorted_nodes[i+1].freq
        
    
    def add_bit(self, c):
        goal_c = c
        if not (c in self.ab):
            goal_c = NULL_CHAR
        cur = self.root
        while cur.c != goal_c:
            next_pos = cur.alph[goal_c]
            if not(c in cur.alph):
                cur.alph[c] = next_pos
            cur.bit.append(bit_to_str[next_pos])
            cur = cur.chd[next_pos]

    def update_leaf(self, c):
        if c in self.ab:
            cur = self.ab[c]
            assert cur.is_leaf()
            cur.freq += 1
            return cur
        cnode = Node(c=c, freq=1)
        self.ab[c] = cnode
        null_node = self.null_node #sortednodes[0]
        null_node_par = null_node.par
        par = Node()
        par.set_chd(null_node, 0)
        par.set_chd(cnode, 1)
        par.alph = {NULL_CHAR: 0, c: 1}
        par.bit = BitArray('0b1')
        if null_node_par:
            null_node_par.set_chd(par, 0 if null_node_par.chd[0] == null_node else 1)
        else:
            self.root = par
        self.sorted_nodes[1:1] = [cnode, par]
        return cnode
    
    def swap_nodes(self, i, j):
        node_i: Node = self.sorted_nodes[i]
        node_j: Node = self.sorted_nodes[j]
        node_lca = self.lca(node_i, node_j)
        self.fix_bits(node_i, node_j, node_lca)
        par_i = node_i.par
        pos_i = 0 if par_i.chd[0] == node_i else 1
        assert par_i.chd[pos_i] == node_i
        par_j = node_j.par
        pos_j = 0 if par_j.chd[0] == node_j else 1
        assert par_j.chd[pos_j] == node_j
        par_i.set_chd(node_j, pos_i)
        par_j.set_chd(node_i, pos_j)
        par_j.freq = par_j.freq + 1
        self.sorted_nodes[i] = node_j
        self.sorted_nodes[j] = node_i
        self.fix_alph_up(node_i, node_lca, node_j.alph)
        self.fix_alph_up(node_j, node_lca, node_i.alph)
        # self.print_huff()

    def lca(self, node_i, node_j):
        global MAX_DISTANCE_LCA
        node_i.freq -= 1
        cur_i: Node = node_i
        cur_j: Node = node_j
        dist = 0
        while cur_i != cur_j:
            while cur_j != cur_i and cur_j.freq <= cur_i.freq:
                dist += 1
                cur_j = cur_j.par
            while cur_i.freq < cur_j.freq:
                dist += 1
                cur_i = cur_i.par
        MAX_DISTANCE_LCA = max(MAX_DISTANCE_LCA, dist)
        node_i.freq += 1
        return cur_i

    def fix_bits(self, node_a: Node, node_b: Node, node_lca: Node):
        saved_bits = node_lca.bit.bin
        # print('swapping', node_a.id, node_b.id, node_lca.id)
        [path_a, pos_a, bit_a] = self.fix_lca_bit(node_a, node_lca, saved_bits)
        [path_b, pos_b, bit_b] = self.fix_lca_bit(node_b, node_lca, saved_bits)
        changes_a = self.get_changes(node_lca, pos_a, bit_a)
        changes_b = self.get_changes(node_lca, pos_b, bit_b)
        self.fix_down(path_a, node_lca, changes_b)
        self.fix_down(path_b, node_lca, changes_a)
    
    def get_changes(self, node_lca: Node, pos, bit_check):
        changes = {}
        count = 0
        i = 0
        for bit in node_lca.bit.bin:
            if bit == str(bit_check):
                if i in pos:
                    if count not in changes:
                        changes[count] = 1
                    else:
                        changes[count] += 1
                else: 
                    count += 1
            i += 1
        # print(pos, bit_check, changes)
        return changes
    
    def fix_lca_bit(self, node_a: Node, node_lca: Node, copy_bin):
        cur = node_a.par
        lst = node_a
        from_idx = '0' if cur.chd[0] == node_a else '1'
        path = [int(from_idx)]
        get_all = True
        old_pos = None
        while cur != node_lca:
            new_pos = set()
            i = 0
            count = 0
            new_bit = BitArray()
            for bit in cur.bit.bin:
                if bit == from_idx:
                    if get_all or count in old_pos:
                        new_pos.add(i)
                    else:
                        new_bit.append(bit_to_str[bit])
                    count += 1
                else:
                    new_bit.append(bit_to_str[bit])
                i += 1
            cur.bit = new_bit
            get_all = False
            nxt = cur.par
            from_idx = '0' if nxt.chd[0] == cur else '1'
            path.append(int(from_idx))
            lst = cur
            cur = nxt
            old_pos = new_pos
        i = 0
        count = 0
        change_to = 0 if lst == node_lca.chd[1] else 1
        changed = []
        for bit in copy_bin:
            if bit == from_idx:
                if get_all or count in old_pos:
                    node_lca.bit.set(change_to, i)
                    changed.append(i)
                count += 1
            i += 1

        return [path, changed, change_to]

    def fix_down(self, path, node: Node, changes):
        cur: Node = node
        # print(node.id, path, changes)
        n = len(path)
        for i in range(n - 1, 0, -1):
            cur = cur.chd[path[i]]
            bit_fill = -1 if path[i - 1] == 1 else 0
            nxt_bit = str(path[i - 1])
            nxt_changes = {}
            idx = 0
            count = 0
            # print('changing', nxt_bit, changes)
            new_bit_array = BitArray()
            for bit in cur.bit.bin:
                if idx in changes:
                    new_bit_array.append(BitArray(int=bit_fill, length=changes[idx]))
                    if count in nxt_changes:
                        nxt_changes[count] += changes[idx]
                    else:
                        nxt_changes[count] = changes[idx]
                    del changes[idx]
                if bit == nxt_bit:
                    count += 1
                new_bit_array.append(bit_to_str[bit])
                idx += 1
            rest = 0
            for idx, value in changes.items():
                rest += value
                new_bit_array.append(BitArray(int=bit_fill, length=value))
            if rest > 0:
                if count in nxt_changes:
                    nxt_changes[count] += rest
                else:
                    nxt_changes[count] = rest
            # print('changed', cur.id, 'from', cur.bit.bin, 'to', new_bit_array.bin)
            # print('nxt_changes:', nxt_changes, rest)
            cur.bit = new_bit_array
            changes = nxt_changes

    def fix_alph_up(self, node_a: Node, node_lca: Node, remove_char):
        dic = node_a.alph
        cur = node_a.par
        from_idx = 0 if cur.chd[0] == node_a else 1
        while cur != node_lca:
            for ch in remove_char:
                del cur.alph[ch]
            for ch in dic:
                cur.alph[ch] = from_idx
            nxt = cur.par
            from_idx = 0 if nxt.chd[0] == cur else 1
            cur = nxt
        for ch in dic:
            node_lca.alph[ch] = from_idx
    
    def assert_wavelet_node(self, root: Node, txt: str):
        if root.is_leaf():
            return
        wavelet = root.bit.bin
        i = 0
        if len(wavelet) != len(txt):
            print("id =", root.id, " with wrong", root.bit.bin, txt)
            assert False
        for ch in txt:
            if str(root.alph[ch]) != wavelet[i]:
                print("id =", root.id, " with wrong ", root.bit.bin, txt)
                assert False
            i += 1
        zero_txt = txt
        one_txt = txt
        for ch, value in root.alph.items():
            if value == 1:
                zero_txt = zero_txt.replace(ch, '')
            else:
                one_txt = one_txt.replace(ch, '')
        if root.chd[0] != None:
            self.assert_wavelet_node(root.chd[0], zero_txt)
        if root.chd[1] != None:
            self.assert_wavelet_node(root.chd[1], one_txt)

    def assert_wavelet(self, txt):
        self.assert_wavelet_node(self.root, txt)

    def print_tree(self, root, level=0):
        if not root:
            return
        print(level*"  ", end='')
        print("[id=%d freq=%d char=%c alph=%s bin=%s]"%(root.id, root.freq, root.c, root.alph, root.bit.bin if root.bit != None else []))
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
    print('MAX DISTANCE TO LCA =', MAX_DISTANCE_LCA)
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

ab = string.ascii_lowercase + ' '
hc, code = encode(input_txt, ab)
print("code =", code)

decoded = decode(code, ab)
print("decoded txt =",decoded)
assert decoded == input_txt


