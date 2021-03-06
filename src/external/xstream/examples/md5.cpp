/*
 * calculates md5 checksum of standard input or file in the 1st parameter
 *
 */

#include <xstream/digest.h>

#include <iostream>
#include <fstream>
#include <cstdio>

using namespace std;;
using namespace xstream;;

int main(int argc, char* argv[]){

	const size_t len = 4*1024;
	char buf[len];

	istream* readfrom;

	try{
		if(1<argc){
			readfrom = new ifstream(argv[1]);
		}else{
			readfrom = &cin;
		}

		digest::md5 md;
		ostream out(&md);

		//raise exceptions
		out.exceptions(ios::badbit);


		while(readfrom->good()){
			readfrom->read(buf,len);
			out.write(buf,readfrom->gcount());
		}

		out.flush();
		struct digest::md5::result res = md.digest();
		cout<<res<<endl;
	}
	catch(exception& e){
		cerr<<"Error: "<<e.what()<<endl;
	}

	//pointers not being freed

	return 0;
}
