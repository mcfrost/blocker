#include <string>
#include <fstream>
#include <iostream>

// 16 noppen 17500
// 12 5000
// 8 12500
// 6 5000
// 4 3000
// 2 2000

//Deckelstein
// 8 2500
// 4 2500


using namespace std;

typedef unsigned char byte;

struct Binvox {
	int dimx,dimy,dimz;
	byte* voxels;
	int getSize() {return dimx*dimy*dimz;};
	byte& getVoxel (int x, int y, int z) {return voxels[x+y*dimx+z*dimx*dimy];};
};

struct Stones {
	int number_of_classes;
	int* stone_length;
	int* stone_count;
	int* internal_lookup;

	Stones (int* g_stones_length, int* g_stones_count, int g_number_of_classes) {
		internal_lookup = new int [g_number_of_classes];
		for (int i = 0; i < g_number_of_classes; ++i) {
			internal_lookup[i] = i;
		}
		stone_length = g_stones_length;
		stone_count = g_stones_count;
		number_of_classes = g_number_of_classes;
	} 

	void remove_stone(int length) {
		int position = internal_lookup[length];
		if ((--stone_count[position]) < stone_count[position + 1]) {
			int tmp_count = stone_count[position];
			int tmp_length = stone_length[position];

			stone_count[position] = stone_count[position + 1];
			stone_length[position] = stone_length[position + 1];
			stone_count[position + 1] = tmp_count;
			stone_length[position + 1] = tmp_length;
			internal_lookup[stone_length[position + 1]] = position;
			internal_lookup[length] = position + 1;
			position++;
		}

		if (stone_count[position] == 0) {
			--number_of_classes;
		}
	}	
};

Binvox read_binvox(string);
void print_layers(Binvox&, std::ostream*);
void optimize_xz_layer(int layer, Binvox&, Stones&);

int main (int argc, char** argv) {
	int* stone_count = {17500,12500,5000,3000};
	int* stone_length = {4,2,3,1};
	Stones stones (stone_length,stone_count,4);
	Binvox b = read_binvox("Apple.binvox");
	for (int i =0; i < b.dimy; ++i) {
		optimize_xz_layer(i,b,stones);
	}
	print_layers(b,(&cout));
	
}

void optimize_line(byte* line,int len,Stones& stones) {
	int no_cluster = 0;
	for (int i = 0; i < len-1; ++i) {
		if (line[i] == 1 && line[i+1] == 0) {
			++no_cluster;
		}
	}

	if (no_cluster == 0) {
		no_cluster = 1;
	}

	int idx = 0;
	int start_idx = 0;

	for (int i=0; i < no_cluster; ++i) {
		
		//move the index pointer and set the start
		while (line[idx] == 0 && idx < len) {
			++idx;
		}
		start_idx = idx;
		
		while (line[idx] == 1 && idx < len) {
			++idx;
		}

		int length = idx - start_idx;

		
	}
}

void optimize_xz_layer_x_direction(int y, Binvox& b,Stones& stones) {
	byte* line = new byte[b.dimx];
	for (int line_no =0; line_no < b.dimz; ++line_no) {
		for (int index_no = 0; index_no < b.dimx; ++index_no) {
			line[index_no] = b.getVoxel(line_no,y,index_no);
		}

		optimize_line(line,b.dimx,stones);

		for (int index_no = 0; index_no < b.dimx; ++index_no) {
			b.getVoxel(line_no,y,index_no) = line[index_no];
		}

	}
}

void optimize_xz_layer_z_direction(int y, Binvox& b, Stones& stones) {
	byte* line = new byte[b.dimz];
	for (int line_no =0; line_no < b.dimx; ++line_no) {
		for (int index_no = 0; index_no < b.dimz; ++index_no) {
			line[index_no] = b.getVoxel(index_no,y,line_no);
		}

		optimize_line(line,b.dimz,stones);

		for (int index_no = 0; index_no < b.dimx; ++index_no) {
			b.getVoxel(index_no,y,line_no) = line[index_no];
		}

	}

}

void optimize_xz_layer(int y, Binvox& b, Stones& stones) {
	if (y % 2 == 0) {
		optimize_xz_layer_x_direction(y,b,stones);
	}
	else {
		optimize_xz_layer_z_direction(y,b,stones);
	}
}


void print_layers (Binvox &b, ostream* out) {
  for (int x =0; x < b.dimx; ++x) {
    for (int z =0; z < b.dimz; ++z) {
      for (int y =0; y < b.dimy; ++y) {
        *out << (char) (b.getVoxel(x,y,z) + 'a') << " ";
      } 
    *out << endl;
    }
  *out << endl;
  }
};

Binvox read_binvox(string filespec)
{

  ifstream *input = new ifstream(filespec.c_str(), ios::in | ios::binary);

  Binvox retval;

  //
  // read header
  //
  string line;
  *input >> line;  // #binvox
  if (line.compare("#binvox") != 0) {
    cout << "Error: first line reads [" << line << "] instead of [#binvox]" << endl;
    delete input;
    return retval;
  }
  int version;
  *input >> version;
  cout << "reading binvox version " << version << endl;

  int depth, height, width;
  depth = -1;
  int done = 0;
  while(input->good() && !done) {
    *input >> line;
    if (line.compare("data") == 0) done = 1;
    else if (line.compare("dim") == 0) {
      *input >> depth >> height >> width;
    }
    else {
      cout << "  unrecognized keyword [" << line << "], skipping" << endl;
      char c;
      do {  // skip until end of line
        c = input->get();
      } while(input->good() && (c != '\n'));

    }
  }
  if (!done) {
    cout << "  error reading header" << endl;
    return retval;
  }
  if (depth == -1) {
    cout << "  missing dimensions in header" << endl;
    return retval;
  }

  int size = width * height * depth;
  byte *voxels = new byte[size];
  if (!voxels) {
    cout << "  error allocating memory" << endl;
    return retval;
  }

  //
  // read voxel data
  //
  byte value;
  byte count;
  int index = 0;
  int end_index = 0;
  int nr_voxels = 0;
  
  input->unsetf(ios::skipws);  // need to read every byte now (!)
  *input >> value;  // read the linefeed char

  while((end_index < size) && input->good()) {
    *input >> value >> count;

    if (input->good()) {
      end_index = index + count;
      if (end_index > size) return retval;
      for(int i=index; i < end_index; i++) voxels[i] = value;
      
      if (value) nr_voxels += count;
      index = end_index;
    }  // if file still ok
    
  }  // while

  input->close();
  cout << "  read " << nr_voxels << " voxels" << endl;
  retval.dimx = width;
  retval.dimy = height;
  retval.dimz = depth;
  retval.voxels = voxels;
  return retval;

}
