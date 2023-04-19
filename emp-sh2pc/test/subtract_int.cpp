#include <typeinfo>
#include "emp-sh2pc/emp-sh2pc.h"
using namespace emp;
using namespace std;

template<typename Op, typename Op2>
void test_int(int party, int range1 = 1<<20, int range2 = 1<<20, int runs = 10000) {
	PRG prg(fix_key);	// Pseudo Random Generator: emp-tool/utils/prg.h
	for(int i = 0; i < runs; ++i) {
		long long ia, ib;
		prg.random_data(&ia, 8);
		prg.random_data(&ib, 8);
		ia %= range1;
		ib %= range2;
		while( Op()(int(ia), int(ib)) != Op()(ia, ib) ) {
			prg.random_data(&ia, 8);
			prg.random_data(&ib, 8);
			ia %= range1;
			ib %= range2;
		}
	
		Integer a(32, ia, ALICE); // Class Integer: emp-tool/circuits/integer.h
		Integer b(32, ib, BOB); // Class Integer: emp-tool/circuits/integer.h

		Integer res = Op2()(a,b);

		if (res.reveal<int>(PUBLIC) != Op()(ia,ib)) {
			cout << ia <<"\t"<<ib<<"\t"<<Op()(ia,ib)<<"\t"<<res.reveal<int>(PUBLIC)<<endl<<flush;
		}
		assert(res.reveal<int>(PUBLIC) == Op()(ia,ib));
	}
	cout << typeid(Op2).name()<<"\t\t\tDONE"<<endl;
}

int main(int argc, char** argv) {
	int port, party;
	parse_party_and_port(argv, &party, &port);
	NetIO * io = new NetIO(party==ALICE ? nullptr : "127.0.0.1", port);

	setup_semi_honest(io, party);

	PRG prg(fix_key);
	int ia = 0;
	int ib = 0;

	if (party == ALICE) ia = 10;
	if (party == BOB) ib = 4;

	Integer a(32, ia, ALICE);
	Integer b(32, ib, BOB);

	Integer res = a - b;
	if (party == ALICE) {
		cout << "Alice: " << ia << endl << flush;
	} else if (party == BOB) {
		cout << "Bob: " << ib << endl << flush;
	}
	cout << "\t" << res.reveal<int>(PUBLIC) << endl << flush;

	finalize_semi_honest();
	delete io;
}


