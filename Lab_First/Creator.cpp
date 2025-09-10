#include <iostream>
#include <fstream>
#include <conio.h>

struct employee {
	int num;
	char name[10];
	double hours;
};

int main(int argc, char* argv[]) {
	int numRecord = atoi(argv[2]);
	employee* workerList = new employee[numRecord];

	std::fstream binaryFile(argv[1], std::ios::binary | std::ios::out);
	std::cout << "Enter list of employees\n";
	std::cout << "#  Number  Name  Hours\n";
	for (int i = 0; i < numRecord; i++) {
		std::cout << i + 1 << ". ";
		std::cin >> workerList[i].num >> workerList[i].name >> workerList[i].hours;
	}

	for (int i = 0; i < numRecord; i++) {
		binaryFile.write((char*)&workerList[i], sizeof(employee));
	}
	_cputs("\nCreator complete his job.\nPress any key to finish...");
	_getch();

	binaryFile.close();

	return 0;
}