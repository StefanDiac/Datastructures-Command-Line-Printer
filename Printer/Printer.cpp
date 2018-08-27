#include <iostream>
#include <conio.h>
#include <fstream>
#include <windows.h>
#include <limits>
#include <string>
//#include <vld.h>
using namespace std;

#pragma region CONSTANTS

static char* const WELCOME_TXT = "Welcome to the printer interface ! \n\n-------------------------------------------------------------\n Please select an option from the following:\n\t 1) Login user\n\t 2) Create user\n\t 3) Change Application Settings (Admin Locked)\n\t 4) Exit Application\n\t Please input a command: ";
static char* const LOGIN_TXT = "Please Log Into Your Account ! \n\n-------------------------------------------------------------\n\t";
static char* const CREATE_NEW_TXT = "Please Add Credentials For This New User ! \n\n-------------------------------------------------------------\n\t";
static char* const ADMIN_OPTIONS_TXT = "Welcome Admin ! \n\n-------------------------------------------------------------\n Please select an option from the following:\n\t 1) Add new printer\n\t 2) Add new color\n\t 3) Add new paperType\n\t 4) Print Through Network\n\t 5) Print Circularly\n\t 6) Change Admin Password\n\t 7) Log Out\n\t Please input a command: ";
static char* const USER_OPTIONS_TXT = "Welcome User ! \n\n-------------------------------------------------------------\n Please select an option from the following:\n\t 1) Add new print task\n\t 2) About\n\t 3) Log Out\n\t Please input a command: ";
static char* ADMIN_PASS = "admin";
static int PRINTER_PRIORITY_CLOSEST_NODES[5] = { -1,-1,-1,-1,-1 };
#pragma endregion

#pragma region Misc
void clear() {
	while (getchar() != '\n');
}
#pragma endregion

#pragma region Types

enum MenuType {
	initialView = 0,
	loginView = 1,
	adminOptionsView = 2,
	createNewView = 3,
	userView = 4,
};

struct MenuView {
	MenuType type;
	char* text;
	int nrOptions;
};

MenuView deepCopyMenu(MenuView menuToAdd) {
	MenuView newMenu;
	newMenu.type = menuToAdd.type;
	newMenu.nrOptions = menuToAdd.nrOptions;
	newMenu.text = (char*)malloc(sizeof(char)*(strlen(menuToAdd.text) + 1));
	strcpy(newMenu.text, menuToAdd.text);
	return newMenu;
}

enum Priority {
	high = 4,
	medium_high = 3,
	medium = 2,
	medium_low = 1,
	low = 0,
};

struct Color {
	char* name;
	char* hexValue;
};

Color deepCopyColor(Color initial) {
	Color newColor;
	newColor.hexValue = (char*)malloc(sizeof(char)*(strlen(initial.hexValue) + 1));
	strcpy(newColor.hexValue, initial.hexValue);
	newColor.name = (char*)malloc(sizeof(char)*(strlen(initial.name) + 1));
	strcpy(newColor.name, initial.name);
	return newColor;
}

struct HashColorNode{
	Color color;
	HashColorNode* next;
};

struct HashTableColors {
	HashColorNode** arrayList;
	int dimension;
};

struct PaperType {
	char* name;
	int width;
	int height;
};

PaperType deepCopyPaper(PaperType initial) {
	PaperType paper;
	paper.name = (char*)malloc(sizeof(char)*(strlen(initial.name) + 1));
	strcpy(paper.name, initial.name);
	paper.width = initial.width;
	paper.height = initial.height;
	return paper;
}

struct HashTablePaper {
	int** sizeMatrix;
	int dimension;
};

struct AppValues {
	//Decide on values
	//Save them to a file upon closing
	//Load them when starting
	const int palletSize = 16;
	int paperSizes;
	int minPaperHeigh;
	int minPaperWidth;
	int maxPaperHeigh;
	int maxPaperWidth;
};

struct Task {
	int taskId;
	Color color;
	PaperType paperType;
	Priority taskPriority;
	char* textToPrint;
};

Task deepCopyTask(Task taskToCopy) {
	Task task;
	task.taskId = taskToCopy.taskId;
	task.taskPriority = taskToCopy.taskPriority;
	task.color = deepCopyColor(taskToCopy.color);
	task.paperType = deepCopyPaper(taskToCopy.paperType);
	task.textToPrint = (char*)malloc(sizeof(char)*(strlen(taskToCopy.textToPrint) + 1));
	strcpy(task.textToPrint, taskToCopy.textToPrint);
	return task;
}

struct PrepareToPrintHeap {
	Task* arrayOfTasks;
	int length;
	int oldLength;
};

struct Printer {
	Task taskToPrint;
	bool occupied;
	Priority priority;
};

struct CircularListNode {
	CircularListNode* previous;
	CircularListNode* next;
	Printer printer;
};

struct SecondaryGraphListNode;

struct MainGraphListNode {
	int nodeCode;
	Printer printer;
	MainGraphListNode* next;
	SecondaryGraphListNode* listStart;
	bool visited;
};

struct SecondaryGraphListNode {
	MainGraphListNode* printerLocation;
	SecondaryGraphListNode* next;
};

#pragma endregion

#pragma region HashTablePaper_Probing

HashTablePaper initializeHashTablePaper(int dimension) {
	HashTablePaper hashTable;
	hashTable.dimension = dimension;
	hashTable.sizeMatrix = (int**)malloc(sizeof(int*)*dimension);
	for (int i = 0; i < dimension; i++) {
		hashTable.sizeMatrix[i] = (int*)malloc(sizeof(int) * 2);
		hashTable.sizeMatrix[i][0] = 0;
		hashTable.sizeMatrix[i][1] = 0;
	}
	return hashTable;
}

int paperHashFunction(PaperType paper, HashTablePaper hashTable, AppValues appValues) {
	int returnValue;
	if (paper.height < appValues.minPaperHeigh || paper.width < appValues.minPaperWidth) {
		return -1;
	}
	else if (paper.height > appValues.maxPaperHeigh || paper.width > appValues.maxPaperWidth) {
		return -2;
	}
	int sum = paper.name[0] + paper.name[1];
	return sum % (hashTable.dimension / 2);
}

HashTablePaper insertPaper(PaperType paper, HashTablePaper hashTable, AppValues appValues) {
	int hashCode = paperHashFunction(paper, hashTable, appValues);
	if (hashCode == -1) {
		cout << "\n\t Paper is invalid. Width x Height is too low. Could not insert.";
	}
	else if (hashCode == -2) {
		cout << "\n\t Paper is invalid. Width x Height is too high. Could not insert.";
	}
	else {
		if (hashTable.sizeMatrix[hashCode][0] == 0) {
			hashTable.sizeMatrix[hashCode][0] = paper.width;
			hashTable.sizeMatrix[hashCode][1] = paper.height;
		}
		else {
			int iterator = hashCode;
			++iterator;
			bool freeSpace = false;
			while (iterator != hashCode && freeSpace == false) {
				if (hashTable.sizeMatrix[iterator][0] == 0) {
					freeSpace = true;
				}
				else {
					if ((iterator + 1) == hashTable.dimension) {
						iterator = 0;
					}
					else {
						++iterator;
					}
				}
			}
			if (freeSpace == true) {
				hashTable.sizeMatrix[iterator][0] = paper.width;
				hashTable.sizeMatrix[iterator][1] = paper.height;
			}
		}
	}
	return hashTable;
}

HashTablePaper deleteHashTable(HashTablePaper hashTable) {
	for (int i = 0; i < hashTable.dimension; i++) {
		free(hashTable.sizeMatrix[i]);
	}
	free(hashTable.sizeMatrix);
	hashTable.sizeMatrix = NULL;
	hashTable.dimension = 0;
	return hashTable;
}

HashTablePaper initFromFile(HashTablePaper hashTable, AppValues appValues) {
	ifstream papers;
	papers.open("papers.txt", ios::in);
	if (papers.is_open()) {
		while (!papers.eof()) {
			int width;
			int height;
			papers >> width >> height;
			PaperType paper;
			
			char name[3] = "";
			name[0] = '0' + (width % 100 / 10);
			name[1] = '0' + (height % 100 / 10);
			name[2] = '\0';
			paper.name = (char*)malloc(sizeof(char) * (strlen(name)+1));
			strcpy(paper.name, name);
			paper.width = width;
			paper.height = height;
			hashTable = insertPaper(paper, hashTable, appValues);
			free(paper.name);
		}
		papers.close();
		
	}
	else {
		throw new exception("Could not open file for papers init !");
	}
	return hashTable;
}

void saveToFile(HashTablePaper hashTable) {
	ofstream papers;
	papers.open("papers.txt", ios::out);
	if (papers.is_open()) {
		for (int i = 0; i < hashTable.dimension; i++) {
			if (hashTable.sizeMatrix[i][0] != 0) {
				papers << hashTable.sizeMatrix[i][0] << " " << hashTable.sizeMatrix[i][1] << endl;
			}
		}
		papers.close();
	}
	else {
		throw new exception("Could not open file for papers init !");
	}
}

HashTablePaper addPaperFromConsole(HashTablePaper hashTable, AppValues appValues) {
	cout << "\n\tPlease input a paper width: ";
	int width;
	cin >> width;
	int height;
	cout << "\n\tPlease input a paper height: ";
	cin >> height;
	PaperType paper;
	char name[3] = "";
	name[0] = '0' + (width % 100 / 10);
	name[1] = '0' + (height % 100 / 10);
	name[2] = '\0';
	paper.name = (char*)malloc(sizeof(char) * (strlen(name) + 1));
	strcpy(paper.name, name);
	paper.width = width;
	paper.height = height;
	hashTable = insertPaper(paper, hashTable, appValues);
	return hashTable;
}

char* paperNameBySize(int width, int height) {
	switch (width) {
	case 26:
		if (height == 37) {
			return "A10";
		}
		else {
			return "Custom";
		}
		break;
	case 37:
		if (height == 52) {
			return "A9";
		}
		else {
			return "Custom";
		}
		break;
	case 52:
		if (height == 74) {
			return "A8";
		}
		else {
			return "Custom";
		}
		break;
	case 74:
		if (height == 105) {
			return "A7";
		}
		else {
			return "Custom";
		}
		break;
	case 105:
		if (height == 148) {
			return "A6";
		}
		else {
			return "Custom";
		}		break;
	case 148:
		if (height == 210) {
			return "A5";
		}
		else {
			return "Custom";
		}
		break;
	case 210:
		if (height == 297) {
			return "A4";
		}
		else {
			return "Custom";
		}
		break;
	case 297:
		if (height == 420) {
			return "A3";
		}
		else {
			return "Custom";
		}
		break;
	case 420:
		if (height == 594) {
			return "A2";
		}
		else {
			return "Custom";
		}
		break;
	case 594:
		if (height == 841) {
			return "A1";
		}
		else {
			return "Custom";
		}
		break;
	case 841:
		if (height == 1189) {
			return "A0";
		}
		else {
			return "Custom";
		}
		break;
	case 1189:
		if (height == 1682) {
			return "2A0";
		}
		else {
			return "Custom";
		}
		break;
	case 1682:
		if (height == 2378) {
			return "4A0";
		}
		else {
			return "Custom";
		}
		break;
	default:
		return "Custom";
	}
}

PaperType selectPaperType(HashTablePaper hashTable) {
	int iterator = 0;
	PaperType returnPaper;
	for (int i = 0; i < hashTable.dimension; i++) {
		if (hashTable.sizeMatrix[i][0] != 0) {
			char displayBuffer[50];
			strcpy(displayBuffer, paperNameBySize(hashTable.sizeMatrix[i][0], hashTable.sizeMatrix[i][1]));
			cout << ++iterator << ") " << displayBuffer<<" ";
		}
	 }
	cout << "\n\tPlease select a paper type from the ones above (Select custom at your discression): ";
	int response;
	cin >> response;
	if (response > 0 && response <= iterator) {
		int secondIterator = 0;
		bool selected = false;
		for (int i = 0; i < hashTable.dimension && selected	== false; i++) {
			if (hashTable.sizeMatrix[i][0] != 0) {
				secondIterator++;
				if (response == secondIterator) {
					returnPaper.height = hashTable.sizeMatrix[i][1];
					returnPaper.width = hashTable.sizeMatrix[i][0];
					char buffer[50];
					strcpy(buffer, paperNameBySize(hashTable.sizeMatrix[i][0], hashTable.sizeMatrix[i][1]));
					returnPaper.name = (char*)malloc(sizeof(char)*(strlen(buffer) + 1));
					strcpy(returnPaper.name, buffer);
					return returnPaper;
				}
			}
		}
	}
	else {
		cout << "\n\tInvalid selection, task creation canceled";
		returnPaper.width = 0;
		return returnPaper;
	}
}

#pragma endregion

#pragma region HashTableColor_Chaining

HashTableColors initialize(int palletSize) {
	HashTableColors hashTable;
	hashTable.dimension = palletSize;
	hashTable.arrayList = (HashColorNode**)malloc(sizeof(HashColorNode*)*hashTable.dimension);
	for (int i = 0; i < hashTable.dimension; i++) {
		hashTable.arrayList[i] = NULL;
	}

	return hashTable;
}

int colorHashFunction(Color color) { 
	char arrayHex[] = "0123456789ABCDEF";
	char* grayScalePtr = strchr(arrayHex, color.hexValue[1]);
	int insertLocation = grayScalePtr - arrayHex;
	return insertLocation;
}

HashColorNode* insertIntoList(Color color, HashColorNode* start) {
	HashColorNode* newNode = (HashColorNode*)malloc(sizeof(HashColorNode));
	newNode->next = NULL;
	newNode->color = deepCopyColor(color);
	if (start) {
		HashColorNode* temporary = start;
		while (temporary->next) {
			temporary = temporary->next;
		}
		temporary->next = newNode;
	}
	else {
		start = newNode;
	}
	return start;
}

HashTableColors insertColor(Color color, HashTableColors hashTable) {
	int hashCode = colorHashFunction(color);
	hashTable.arrayList[hashCode] = insertIntoList(color, hashTable.arrayList[hashCode]);
	return hashTable;
}

HashColorNode* deleteHashTableListColors(HashColorNode* start) {
	while (start) {
		free(start->color.hexValue);
		free(start->color.name);
		HashColorNode* temp = start;
		start = start->next;
		free(temp);
	}
	return start;
}

HashTableColors deleteHashTableColors(HashTableColors hashTable) {
	for (int i = 0; i < hashTable.dimension; i++) {
		hashTable.arrayList[i] = deleteHashTableListColors(hashTable.arrayList[i]);
	}
	free(hashTable.arrayList);
	hashTable.arrayList = NULL;
	hashTable.dimension = 0;
	return hashTable;
}

HashTableColors reinstanceFromSave(HashTableColors hashTable) {
	if (hashTable.arrayList) {
		ifstream colors;
		colors.open("colors.txt", ios::in);
		if (colors.is_open()) {
			while (!colors.eof()) {
				char nameBuffer[50];
				char hexBuffer[50];
				colors >> nameBuffer >> hexBuffer;
				if (strlen(nameBuffer)>=1) {
					Color color;
					color.name = (char*)malloc(sizeof(char)*(strlen(nameBuffer) + 1));
					color.hexValue = (char*)malloc(sizeof(char)*(strlen(hexBuffer) + 1));
					strcpy(color.name, nameBuffer);
					strcpy(color.hexValue, hexBuffer);
					hashTable = insertColor(color, hashTable);
					free(color.name);
					free(color.hexValue);
				}
			}
			colors.close();
		}
		else {
			throw new exception("Could not open file for colors !");
		}
	}
	return hashTable;
}

void saveColorsToFile(HashTableColors hashTable) {
	if (hashTable.arrayList) {
		ofstream colors;
		colors.open("colors.txt", ios::out);
		if (colors.is_open()) {
			for (int i = 0; i < hashTable.dimension; i++) {
				if (hashTable.arrayList[i]) {
					HashColorNode* temp = hashTable.arrayList[i];
					while (temp) {
						char nameBuffer[50];
						char hexBuffer[50];
						strcpy(nameBuffer, temp->color.name);
						strcpy(hexBuffer, temp->color.hexValue);
						colors << nameBuffer << " " << hexBuffer << endl;
						temp = temp->next;
					}
				}
			}
			colors.close();
		}
		else {
			throw new exception("Could not open file for colors !");
		}
	}
}

bool verifyHexvalue(char hexValue[50]) {
	bool correct = true;
	char arrayHex[] = "#0123456789ABCDEFabcdef";
	if (strlen(hexValue) != 7) {
		correct = false;
	}
	else{
		if (hexValue[0] != '#') {
			correct = false;
		}
		for (int i = 0; i < strlen(hexValue) && correct; i++) {
			if (strchr(arrayHex, hexValue[i])==NULL) {
				correct = false;
			}
		}
	}
	return correct;
}

HashTableColors addColorFromConsole(HashTableColors hashTable) {
	if (hashTable.arrayList) {
		char colorName[50];
		cout << "\n\tColor name: ";
		cin.getline(colorName, 50);
		int retryCounter = 0;
		bool correct = false;
		char colorHex[50];
		do {
			
			cout << "\n\tColor Hex Value: ";
			cin.getline(colorHex, 50);
			correct = verifyHexvalue(colorHex);
			cout << "\n\tInvalid Value entered. Please stick to type #FFFFFF";
			retryCounter++;
		} while (retryCounter <= 3 && correct == false);
		if (correct) {
			Color color;
			color.hexValue = (char*)malloc(sizeof(char)*(strlen(colorHex) + 1));
			color.name = (char*)malloc(sizeof(char)*(strlen(colorName) + 1));
			strcpy(color.hexValue, colorHex);
			strcpy(color.name, colorName);
			hashTable = insertColor(color, hashTable);
		}
		else {
			"\n\tMaximum number of retries reached.";
			Sleep(500);
		}
	}
	else {
		cout << "\n\tHashtable has not been initialized !";
	}
	return hashTable;
}

Color selectColorType(HashTableColors hashTable) {
	int iterator = 0;
	for (int i = 0; i < hashTable.dimension; i++) {
		if (hashTable.arrayList[i] != NULL) {
			HashColorNode* temp = hashTable.arrayList[i];
			while (temp) {
				cout << ++iterator << ") " << temp->color.name<<" ";
				temp = temp->next;
			}
		}
	}
	cout << "\n\tWhich color would you like to select: ";
	int response;
	cin >> response;
	if (response > 0 && response <= iterator) {
		int secondIterator = 0;
		bool found = false;
		for (int i = 0; i < hashTable.dimension && found == false; i++) {
			if (hashTable.arrayList[i] != NULL) {
				HashColorNode* temp = hashTable.arrayList[i];
				while (temp) {
					secondIterator++;
					if (secondIterator == response) {
						found = true;
						Color color;
						color.hexValue = (char*)malloc(sizeof(char)*(strlen(temp->color.hexValue) + 1));
						color.name = (char*)malloc(sizeof(char)*(strlen(temp->color.name) + 1));
						strcpy(color.hexValue, temp->color.hexValue);
						strcpy(color.name, temp->color.name);
						return color;
					}
					temp = temp->next;
				}
			}
		}
	}
	else {
		Color color;
		color.hexValue = (char*)malloc(sizeof(char)*(strlen("N/A") + 1));
		strcpy(color.hexValue, "N/A");
		return color;
	}
}

#pragma endregion

#pragma region TaskBinaryTree

struct BinaryTreeNode {
	Task nodeTask;
	BinaryTreeNode* right;
	BinaryTreeNode* left;
};

BinaryTreeNode* addTaskToTree(BinaryTreeNode* root, Task taskToAdd) {
	if (root) {
		if (root->nodeTask.taskId > taskToAdd.taskId) {
			root->left = addTaskToTree(root->left, taskToAdd);
		}
		else {
			root->right = addTaskToTree(root->right, taskToAdd);
		}
		return root;
	}
	else {
		BinaryTreeNode* newNode = (BinaryTreeNode*)malloc(sizeof(BinaryTreeNode));
		newNode->nodeTask = deepCopyTask(taskToAdd);
		newNode->left = NULL;
		newNode->right = NULL;
		return newNode;
	}
}

BinaryTreeNode* deleteBinaryTree(BinaryTreeNode* root) {
	if (root) {
		root->left = deleteBinaryTree(root->left);
		root->right = deleteBinaryTree(root->right);
		free(root->nodeTask.textToPrint);
		free(root->nodeTask.color.hexValue);
		free(root->nodeTask.color.name);
		free(root->nodeTask.paperType.name);
		free(root);

		return NULL;
	}
}

Task createTaskFromConsole(HashTableColors hashTableColors, HashTablePaper hashTablePapers) {
	Task task;
	cout << "\n\t Please add a task id: ";
	cin >> task.taskId;
	if (task.taskId < 0) {
		cout << "Invalid ID added. Id must be a natural number. Task creation failed";
		task.taskId = -1;
		return task;
	}
	task.color = selectColorType(hashTableColors);
	if (strcmp(task.color.hexValue, "N/A") == 0) {
		cout << "Invalid color created. Task creation failed";
		task.taskId = -1;
		return task;
	}
	task.paperType = selectPaperType(hashTablePapers);
	if (task.paperType.width == 0) {
		task.taskId = -1;
		return task;
	}
	cout << "\n\t Please select a level of priority for the task:\n\t 1) high 2) medium high 3) medium 4) medium low 5) low\n";
	int response;
	cin >> response;
	if (response > 0 && response <= 5) {
		switch (response) {
		case 1:
			task.taskPriority = high;
			break;
		case 2:
			task.taskPriority = medium_high;
			break;
		case 3:
			task.taskPriority = medium;
			break;
		case 4:
			task.taskPriority = medium_low;
			break;
		case 5:
			task.taskPriority = low;
			break;
		}
		cout << "\n\t Please add the print task text (character limit 250): ";
		char buffer[250];
		int ch;
		while ((ch = cin.get()) != '\n' && ch != EOF);
		cin.getline(buffer, 250);
		task.textToPrint = (char*)malloc(sizeof(char)*(strlen(buffer) + 1));
		strcpy(task.textToPrint, buffer);
		return task;
	}
	else {
		cout << "Invalid task priority ! Task creation failed";
		task.taskId = -1;
		return task;
	}
}

void getTasksArray(Task* &taskArray, BinaryTreeNode* root, int &iterator) {
	if (root) {
		getTasksArray(taskArray, root->left,iterator);
		taskArray[iterator++] = deepCopyTask(root->nodeTask);
		getTasksArray(taskArray, root->right, iterator);
	}
}

BinaryTreeNode* deleteEntry(BinaryTreeNode* root, int taskIdToDelete) {
	if (root) {
		if (taskIdToDelete < root->nodeTask.taskId) {
			root->left = deleteEntry(root->left, taskIdToDelete);
		}
		else if (taskIdToDelete > root->nodeTask.taskId) {
			root->right = deleteEntry(root->right, taskIdToDelete);
		}
		else {
			if (root->left == NULL) {
				BinaryTreeNode* temp = root;
				root = root->right;
				free(temp->nodeTask.color.hexValue);
				free(temp->nodeTask.color.name);
				free(temp->nodeTask.paperType.name);
				free(temp->nodeTask.textToPrint);
				free(temp);
			}
			else if (root->right == NULL) {
				BinaryTreeNode* temp = root;
				root = root->left;
				free(temp->nodeTask.color.hexValue);
				free(temp->nodeTask.color.name);
				free(temp->nodeTask.paperType.name);
				free(temp->nodeTask.textToPrint);
				free(temp);
			}
			else {
				BinaryTreeNode* auxiliary = root->right;
				while (auxiliary->left) {
					auxiliary = auxiliary->left;
				}
				root->nodeTask = deepCopyTask(auxiliary->nodeTask);
				root->right = deleteEntry(root->right, auxiliary->nodeTask.taskId);
			}
		}
		return root;
	}
}

#pragma endregion

#pragma region PriorityTaskHeap

void filterTasks(PrepareToPrintHeap heap, int position) {
	int leftPosition = position * 2 + 1;
	int rightPosition = position * 2 + 2;
	int maxPosition = position;

	if (leftPosition < heap.length && heap.arrayOfTasks[leftPosition].taskPriority>heap.arrayOfTasks[maxPosition].taskPriority) {
		maxPosition = leftPosition;
	}

	if (rightPosition < heap.length && heap.arrayOfTasks[rightPosition].taskPriority > heap.arrayOfTasks[maxPosition].taskPriority) {
		maxPosition = rightPosition;
	}

	if (maxPosition != position) {
		Task auxiliary = deepCopyTask(heap.arrayOfTasks[maxPosition]);
		heap.arrayOfTasks[maxPosition] = deepCopyTask(heap.arrayOfTasks[position]);
		heap.arrayOfTasks[position] = deepCopyTask(auxiliary);

		free(auxiliary.color.hexValue);
		free(auxiliary.color.name);
		free(auxiliary.paperType.name);
		free(auxiliary.textToPrint);

		if (maxPosition <= (heap.length - 2) / 2) {
			filterTasks(heap, maxPosition);
		}
	}
}

PrepareToPrintHeap insertTasks(BinaryTreeNode* root, PrepareToPrintHeap heap) {
	if (heap.length != 0) {
		if (heap.arrayOfTasks != NULL) {
			for (int i = 0; i < heap.oldLength; i++) {
				free(heap.arrayOfTasks[i].color.name);
				free(heap.arrayOfTasks[i].color.hexValue);
				free(heap.arrayOfTasks[i].paperType.name);
				free(heap.arrayOfTasks[i].textToPrint);
			}
			free(heap.arrayOfTasks);
			heap.arrayOfTasks = NULL;
		}
		heap.oldLength = heap.length;
		heap.arrayOfTasks = (Task*)malloc(sizeof(Task)*heap.length);
		int iterator = 0;
		getTasksArray(heap.arrayOfTasks, root, iterator);
		for (int i = (heap.length - 2) / 2; i >= 0; i--) {
			filterTasks(heap, i);
		}
	}
	return heap;
}

Task extractPriorityTask(PrepareToPrintHeap &heap) {
	if (heap.length != 0) {
		Task result = deepCopyTask(heap.arrayOfTasks[0]);

		Task* temp = (Task*)malloc(sizeof(Task)*(heap.length - 1));
		for (int i = 1; i < heap.length; i++) {
			temp[i - 1] = deepCopyTask(heap.arrayOfTasks[i]);
		}
		for (int i = 0; i < heap.length; i++) {
			free(heap.arrayOfTasks[i].color.name);
			free(heap.arrayOfTasks[i].color.hexValue);
			free(heap.arrayOfTasks[i].paperType.name);
			free(heap.arrayOfTasks[i].textToPrint);
		}
		free(heap.arrayOfTasks);

		heap.arrayOfTasks = temp;
		--heap.length;
		--heap.oldLength;

		for (int i = (heap.length - 2) / 2; i >= 0; i--) {
			filterTasks(heap, i);
		}

		return result;
	}
	else {
		Task result;
		result.taskId = -1;
		return result;
	}
}

#pragma endregion

#pragma region Circular_Printer_List

CircularListNode* insertPrinterToList(CircularListNode* startingNode, Priority priority) {
	CircularListNode* newNode = (CircularListNode*)malloc(sizeof(CircularListNode));
	newNode->printer.priority = priority;
	newNode->printer.occupied = false;
	if (startingNode) {
		if (priority >= medium) {
			CircularListNode* temp = startingNode;
			while (temp->next != startingNode&&temp->next->printer.priority >= newNode->printer.priority) {
				temp = temp->next;
			}
			newNode->next = temp->next;
			newNode->previous = temp;
			temp->next->previous = newNode;
			temp->next = newNode;
			return startingNode;
		}
		else {
			CircularListNode* temp = startingNode;
			while (temp->next != startingNode&&temp->previous->printer.priority < newNode->printer.priority) {
				temp = temp->previous;
			}
			newNode->next = temp;
			newNode->previous = temp->previous;
			temp->previous->next = newNode;
			temp->previous = newNode;
		}
	}
	else {
		newNode->next = newNode;
		newNode->previous = newNode;
		startingNode = newNode;
		return startingNode;
	}
}

bool attributeTaskCircular(Task taskToPrint, CircularListNode* startingNode) {
	CircularListNode* temp = startingNode;
	bool foundPrinter = false;
	if (taskToPrint.taskPriority >= medium) {
		do {
			if (temp->printer.priority <= taskToPrint.taskPriority&&temp->printer.occupied == false) {
				foundPrinter = true;
				temp->printer.occupied = true;
				temp->printer.taskToPrint = deepCopyTask(taskToPrint);
			}
			else {
				temp = temp->next;
			}
		} while (foundPrinter == false && temp != startingNode);
	}
	else {
		do {
			if (temp->printer.priority == taskToPrint.taskPriority&&temp->printer.occupied == false) {
				foundPrinter = true;
				temp->printer.occupied = true;
				temp->printer.taskToPrint = deepCopyTask(taskToPrint);
			}
			else {
				temp = temp->previous;
			}
		} while (foundPrinter == false && temp != startingNode);
	}
	if (foundPrinter == false) {
		cout << "Could not find a printer with for the task. All printers might be full, or no printer for the low priority task is available";
	}
	return foundPrinter;
}

void printCircular(CircularListNode* startingNode) {
	if (startingNode != NULL) {
		CircularListNode* temp = startingNode;
		ofstream prints;
		prints.open("prints.txt", ios::out | ios::app);
		if (prints.is_open()) {
			do {
				if (temp->printer.occupied) {
					prints << "Printed circularly by printer with priority " << temp->printer.priority << endl;
					prints << "About Task " << temp->printer.taskToPrint.taskId << endl;
					prints << "--------------------------------------------------" << endl;
					prints << "Color - " << temp->printer.taskToPrint.color.name << " " << temp->printer.taskToPrint.color.hexValue << endl;
					prints << "Paper - " << temp->printer.taskToPrint.paperType.width << "x" << temp->printer.taskToPrint.paperType.height << endl;
					prints << "Text - " << temp->printer.taskToPrint.textToPrint << endl;
					prints << "--------------------------------------------------" << endl;

					temp->printer.occupied = false;
					free(temp->printer.taskToPrint.color.hexValue);
					free(temp->printer.taskToPrint.color.name);
					free(temp->printer.taskToPrint.paperType.name);
					free(temp->printer.taskToPrint.textToPrint);
				}
				temp = temp->next;
			} while (temp != startingNode);
			prints.close();
		}
	}
}

void deleteCircularList(CircularListNode* &startingNode) {
	if (startingNode) {
		CircularListNode* temp = startingNode->next;
		while (temp != startingNode) {
			CircularListNode* toDelete = temp;
			temp = temp->next;
			if (toDelete->printer.occupied) {
				free(toDelete->printer.taskToPrint.color.hexValue);
				free(toDelete->printer.taskToPrint.color.name);
				free(toDelete->printer.taskToPrint.paperType.name);
				free(toDelete->printer.taskToPrint.textToPrint);
			}
			free(toDelete);
		}
		free(startingNode);
		startingNode = NULL;
	}
}

void initPrintersFromFile(CircularListNode* &startingNode) {
	ifstream printers;
	printers.open("printers.txt", ios::in);
	int priorityToRead;
	if (printers.is_open()) {
		while (!printers.eof()) {
			printers >> priorityToRead;
			switch (priorityToRead) {
			case 0:
				startingNode = insertPrinterToList(startingNode, low);
				break;
			case 1:
				startingNode = insertPrinterToList(startingNode, medium_low);
				break;
			case 2:
				startingNode = insertPrinterToList(startingNode, medium);
				break;
			case 3:
				startingNode = insertPrinterToList(startingNode, medium_high);
				break;
			case 4:
				startingNode = insertPrinterToList(startingNode, high);
				break;
			}
		}
		printers.close();
	}
}

void saveNewPrinterToFile(Priority priorityLevel) {
	ofstream printers;
	printers.open("printers.txt", ios::out | ios::app);
	if (printers.is_open()) {
		switch (priorityLevel) {
		case low:
			printers << 0 << endl;
			break;
		case medium_low:
			printers << 1 << endl;
			break;
		case medium:
			printers << 2 << endl;
			break;
		case medium_high:
			printers << 3 << endl;
				break;
		case high:
			printers << 4 << endl;
			break;
		}
		printers.close();
	}
}

#pragma endregion

#pragma region Graph_Printer_List

MainGraphListNode* insertPrinterToMainList(MainGraphListNode* mainListStart, Priority priority) {
	MainGraphListNode* newNode = (MainGraphListNode*)malloc(sizeof(MainGraphListNode));
	newNode->listStart = NULL;
	newNode->next = NULL;
	newNode->printer.occupied = false;
	newNode->visited = 0;
	newNode->printer.priority = priority;
	
	if (mainListStart) {
		int counter = 2;
		MainGraphListNode* temp = mainListStart;
		while (temp->next) {
			temp = temp->next;
			counter++;
		}
		temp->next = newNode;
		newNode->nodeCode = counter;
	}
	else {
		mainListStart = newNode;
		newNode->nodeCode = 1;
	}

	if (PRINTER_PRIORITY_CLOSEST_NODES[(int)newNode->printer.priority] == -1) {
		PRINTER_PRIORITY_CLOSEST_NODES[(int)newNode->printer.priority] = newNode->nodeCode;
	}

	return mainListStart;
}

SecondaryGraphListNode* insertIntoSecondaryList(SecondaryGraphListNode* startNode, int nodeCode, MainGraphListNode* caller, MainGraphListNode*& mainList) {
	SecondaryGraphListNode* newNode = (SecondaryGraphListNode*)malloc(sizeof(SecondaryGraphListNode));
	newNode->next = NULL;
	MainGraphListNode* temp = mainList;
	MainGraphListNode* address;
	bool addressFound = false;
	while (temp && addressFound==false) {
		if (temp->nodeCode == nodeCode) {
			addressFound = true;
			address = temp;
		}
		else {
			temp = temp->next;
		}
	}
	newNode->printerLocation = address;

	if (startNode) {
		SecondaryGraphListNode* temp = startNode;
		while (temp->next) {
			temp = temp->next;
		}
		temp->next = newNode;
	}
	else {
		startNode = newNode;
	}

	SecondaryGraphListNode* newNodeSecond = (SecondaryGraphListNode*)malloc(sizeof(SecondaryGraphListNode));
	newNodeSecond->next = NULL;
	newNodeSecond->printerLocation = caller;
	if (address->listStart) {
		SecondaryGraphListNode* temp = address->listStart;
		while (temp->next) {
			temp = temp->next;
		}
		temp->next = newNodeSecond;
	}
	else {
		address->listStart = newNodeSecond;
	}
	return startNode;
}

MainGraphListNode* initFromFile(MainGraphListNode* listStart, bool &worked) {
	ifstream printers;
	printers.open("printers.txt", ios::in);
	worked = true;
	int priorityToRead;
	if (printers.is_open()) {
		while (printers>>priorityToRead) {
			switch (priorityToRead) {
			case 0:
				listStart = insertPrinterToMainList(listStart, low);
				break;
			case 1:
				listStart = insertPrinterToMainList(listStart, medium_low);
				break;
			case 2:
				listStart = insertPrinterToMainList(listStart, medium);
				break;
			case 3:
				listStart = insertPrinterToMainList(listStart, medium_high);
				break;
			case 4:
				listStart = insertPrinterToMainList(listStart, high);
				break;
			}
		}
		printers.close();
	}

	ifstream printerRelations;
	printerRelations.open("printerRelations.txt", ios::in);
	int code1;
	int code2;
	if (printerRelations.is_open()) {
		while (!printerRelations.eof()) {
			printerRelations >> code1 >> code2;
			MainGraphListNode* temp = listStart;
			bool found = false;
			while (temp!=NULL && found == false) {
				if (temp->nodeCode == code1) {
					found = true;
				}
				else {
					temp = temp->next;
				}
			}
			if (found == false) {
				worked = false;
			}
			else {
				temp->listStart = insertIntoSecondaryList(temp->listStart, code2, temp, listStart);
			}
		}
		printerRelations.close();
	}
	return listStart;
}

int invertNumber(int number) {
	int inverted = 0;
	int p = 1;
	while (number) {
		inverted = inverted*p + number % 10;
		number = number / 10;
		p = p * 10;
	}
	return inverted;
}

MainGraphListNode* deleteGraphMain(MainGraphListNode* listStart) {
	MainGraphListNode* temp = listStart;
	while (temp) {
		if (temp->listStart != NULL) {
			SecondaryGraphListNode* temp2 = temp->listStart;
			while (temp2) {
				SecondaryGraphListNode* deleteThis = temp2;
				temp2 = temp2->next;
				free(deleteThis);
			}
			temp->listStart = NULL;
		}
		MainGraphListNode* deleteThis = temp;
		temp = temp->next;
		free(deleteThis);
	}
	listStart = NULL;
	return listStart;
}

char* printToConsolePrintersForPriority(char* option,Priority priority, MainGraphListNode* listStart, bool& found) {
	char options[50] = { '#' };
	int iterator=0;
	if (listStart) {
		MainGraphListNode* temp = listStart;
		while (temp) {
			if (temp->printer.priority == priority) {
				cout << temp->nodeCode << ") ";
				found = true;
				int optionToAdd = temp->nodeCode;
				string invertedNumber = std::to_string(optionToAdd);
				if (options[0] == '#') {
					if (optionToAdd > 9) {
						for (char& c : invertedNumber) {
							if (c != ' ') {
								options[iterator] = c;
								iterator++;
							}
						}
						options[iterator] = ' ';
						iterator++;
					}
					else {
						options[0] = optionToAdd + '0';
						iterator++;
						options[iterator] = ' ';
						iterator++;
					}
					
				}
				else {
					if (optionToAdd > 9) {
						for (char& c : invertedNumber) {
							if (c != ' ') {
								options[iterator] = c;
								iterator++;
							}
						}
						options[iterator] = ' ';
						iterator++;
					}
					else {
						options[iterator] = optionToAdd + '0';
						iterator++;
						options[iterator] = ' ';
						iterator++;
					}
				}
				invertedNumber.clear();
			}
			temp = temp->next;
		}
	}
	options[iterator] = '\0';
	option = (char*)malloc(sizeof(char)*(strlen(options) + 1));
	strcpy(option, options);
	return option;
}

void findGraphNode(MainGraphListNode* &nodeToCheck, int nodeCodeToFind, Task taskToPrint, bool &found) {
	nodeToCheck->visited = true;
	if (nodeToCheck->nodeCode == nodeCodeToFind) {
		found = true;
		ofstream prints;
		prints.open("prints.txt", ios::out | ios::app);
		if (prints.is_open()) {
			prints << "Printed using a graph by printer with priority " << taskToPrint.taskPriority << endl;
			prints << "About Task " << taskToPrint.taskId << endl;
			prints << "--------------------------------------------------" << endl;
			prints << "Color - " << taskToPrint.color.name << " " << taskToPrint.color.hexValue << endl;
			prints << "Paper - " << taskToPrint.paperType.width << "x" << taskToPrint.paperType.height << endl;
			prints << "Text - " << taskToPrint.textToPrint << endl;
			prints << "--------------------------------------------------" << endl;
			prints.close();
		}
	}
	else {
		SecondaryGraphListNode* tempSec = nodeToCheck->listStart;

		while (tempSec && found == false) {
			if (tempSec->printerLocation->visited == false) {
				findGraphNode(tempSec->printerLocation, nodeCodeToFind, taskToPrint, found);
			}
			tempSec = tempSec->next;
		}
	}
}

void printUsingGraph(MainGraphListNode* &listStart, int nodeCodeToFind, Task taskToPrint) {
	MainGraphListNode* temp = listStart;
	int startCode = PRINTER_PRIORITY_CLOSEST_NODES[(int)taskToPrint.taskPriority];
	while (temp->nodeCode != startCode) {
		temp = temp->next;
	}
	bool found = false;
	if (nodeCodeToFind == startCode) {
		ofstream prints;
		prints.open("prints.txt", ios::out | ios::app);
		if (prints.is_open()) {
			prints << "Printed using a graph by printer with priority " << taskToPrint.taskPriority << endl;
			prints << "About Task " << taskToPrint.taskId << endl;
			prints << "--------------------------------------------------" << endl;
			prints << "Color - " << taskToPrint.color.name << " " << taskToPrint.color.hexValue << endl;
			prints << "Paper - " << taskToPrint.paperType.width << "x" << taskToPrint.paperType.height << endl;
			prints << "Text - " << taskToPrint.textToPrint << endl;
			prints << "--------------------------------------------------" << endl;
			prints.close();
		}
	}
	else {
		SecondaryGraphListNode* tempSec = temp->listStart;
		temp->visited = true;
		while (tempSec&&found == false) {
			findGraphNode(tempSec->printerLocation, nodeCodeToFind, taskToPrint, found);
			tempSec = tempSec->next;
		}
		if (found == false) {
			cout << "\n\tPrinting failed ! Could not find the printer. Please check if printer is connected to main printer !";
			Sleep(1000);
		}
		else {
			cout << "\n\tPrinted !";
			Sleep(200);
		}
	}
	
}

void resetGraphVisited(MainGraphListNode* &listStart) {
	MainGraphListNode* temp = listStart;
	while (temp) {
		temp->visited = false;
		temp = temp->next;
	}
}

void reprintChoices(MainGraphListNode* listStart, int lastNode, Priority priority) {
	MainGraphListNode* temp;
	temp = listStart;
	cout << endl;
	while (temp) {
		if (temp->nodeCode != lastNode && temp->printer.priority == priority && temp->visited == false) {
			cout << temp->nodeCode << ") ";
		}
		temp = temp->next;
	}
}

MainGraphListNode* addLinks(MainGraphListNode* listStart, Priority priority) {
	MainGraphListNode* temp = listStart;
	int lastNode = 0;
	MainGraphListNode* lastAddress = NULL;
	while (temp) {
		lastNode++;
		if (temp->next == NULL) {
			lastAddress = temp;
		}
		temp = temp->next;
	}
	temp = listStart;
	cout << endl;
	while (temp) {
		if (temp->nodeCode != lastNode && temp->printer.priority == priority) {
			cout << temp->nodeCode << ") ";
		}
		temp = temp->next;
	}
	int retries = 0;
	
	cout << endl;
	bool possible = true;
	bool atLeastOne = false;
	int option;
	while (possible == true && retries <= 5) {
		temp = listStart;
		bool found = false;
		if (atLeastOne) {
			cout << "\n\tPlease select one of the above printers to connect to the new printer (-1 to stop) : ";
			cin >> option;
			if (option == -1) {
				possible = false;
				cout << "\n\tDone adding connections";
			}
			else if (option <= 0 && option>lastNode) {
				cout << "\n\tInvalid option ! Retries remaining " << 5-retries;
				retries++;
			}
			else {
				while (temp != NULL && found == false) {
					if (temp->visited == false && temp->printer.priority == priority && temp->nodeCode==option) {
						lastAddress->listStart = insertIntoSecondaryList(lastAddress->listStart, temp->nodeCode, lastAddress, listStart);
						found = true;
						temp->visited = true;
						ofstream printerRelations;
						printerRelations.open("printerRelations.txt", ios::out | ios::app);
						if (printerRelations.is_open()) {
							printerRelations << temp->nodeCode << " " << lastAddress->nodeCode << endl;
						}
						else {
							cout << "\n\tCould not save to file";
						}
						printerRelations.close();
						reprintChoices(listStart, lastNode, priority);
					}
					else if (temp->nodeCode == lastNode) {
						possible = false;
						cout << "\n\tExhausted all possible connections to be made";
					}
					else if (temp->nodeCode == option && temp->printer.priority != priority) {
						cout << "\n\tInvalid option ! Retries remaining " << 5 - retries;
						found = true;
						retries++;
					}
					else if (temp->nodeCode == option && temp->printer.priority == priority && temp->visited == true) {
						found = true;
						cout << "\n\tLink already exists ! Retries remaining " << 5 - retries;
						retries++;
					}
					temp = temp->next;
				}
			}
		}
		else {
			cout << "\n\tPlease select one of the above printers to connect to the new printer : (If none were printed, that means that it is a new printer for this priority !";
			cin >> option;
			if (option <= 0 && option>lastNode) {
				cout << "\n\tInvalid option ! You must add at least one connection to the new printer !";
			}
			else {
				while (temp != NULL && found == false) {
					if (temp->visited == false && temp->printer.priority == priority && temp->nodeCode == option) {
						atLeastOne = true;
						lastAddress->listStart = insertIntoSecondaryList(lastAddress->listStart, temp->nodeCode, lastAddress, listStart);
						found = true;
						temp->visited = true;
						ofstream printerRelations;
						printerRelations.open("printerRelations.txt", ios::out | ios::app);
						if (printerRelations.is_open()) {
							printerRelations << temp->nodeCode << " " << lastAddress->nodeCode<<endl;
						}
						else {
							cout << "\n\tCould not save to file";
						}
						printerRelations.close();
						reprintChoices(listStart, lastNode, priority);
					}
					else if (temp->nodeCode == lastNode) {
						possible = false;
						cout << "\n\tNo printers of this type exist. Added new printer !";
					}
					else if (temp->nodeCode == option && temp->printer.priority != priority) {
						cout << "\n\tInvalid option !";
						found = true;
					}
					else if (temp->nodeCode == option && temp->printer.priority == priority && temp->visited == true) {
						found = true;
						cout << "\n\tLink already exists !";
					}
					temp = temp->next;
				}
			}
			
		}
	}
	if (retries > 5) {
		cout << "\n\tMaximum amount of retries reached ! Exiting...";
		Sleep(500);
	}
	return listStart;
}


#pragma endregion

#pragma region MenuStack

struct MenuStackNode {
	MenuStackNode* next;
	MenuStackNode* previous;
	MenuView view;
};

struct MenuStackController {
	MenuStackNode* head;
	MenuStackNode* tail;
};

MenuStackController addMenu(MenuStackController myMenuStack, MenuView menuToAdd) {

	MenuStackNode* newNodeItem = (MenuStackNode*)malloc(sizeof(MenuStackNode));
	newNodeItem->next = NULL;
	newNodeItem->view = deepCopyMenu(menuToAdd);
	newNodeItem->previous = myMenuStack.tail;
	if (myMenuStack.tail) {
		myMenuStack.tail->next = newNodeItem;
	}
	else {
		myMenuStack.head = newNodeItem;
	}
	myMenuStack.tail = newNodeItem;
	return myMenuStack;
}

int displayCurrentMenu(MenuStackNode* currentView) {
	system("cls");
	cout<<currentView->view.text;
	int option = 0;
	int resetCounter = 0;
	bool correct = false;
	while (!correct) {
		scanf("%d", &option);
		clear();
		if (option > 0 && option <= currentView->view.nrOptions)
			correct = true;
		else
		{
			printf("Invalid option ! \n Please input another option: ");
			resetCounter++;
		}
		if (resetCounter >= 5) {
			system("cls");
			cout << currentView->view.text;
			resetCounter = 0;
		}
	}
	return option;
}

MenuView dynamicMenuCreation(MenuType type) {
	MenuView myView;
	switch (type) {
	case initialView:
		myView.nrOptions = 4;
		myView.text = (char*)malloc(sizeof(char)*(strlen(WELCOME_TXT) + 1));
		strcpy(myView.text, WELCOME_TXT);
		myView.type = initialView;
		break;
	case loginView:
		myView.nrOptions = 0;
		myView.text = (char*)malloc(sizeof(char)*(strlen(LOGIN_TXT) + 1));
		myView.type = loginView;
		strcpy(myView.text, LOGIN_TXT);
		break;
	case adminOptionsView:
		myView.nrOptions = 7;
		myView.text = (char*)malloc(sizeof(char)*(strlen(ADMIN_OPTIONS_TXT) + 1));
		myView.type = adminOptionsView;
		strcpy(myView.text, ADMIN_OPTIONS_TXT);
		break;
	case createNewView:
		myView.nrOptions = 0;
		myView.text = (char*)malloc(sizeof(char)*(strlen(CREATE_NEW_TXT) + 1));
		myView.type = createNewView;
		strcpy(myView.text, CREATE_NEW_TXT);
		break;
	case userView:
		myView.nrOptions = 3;
		myView.text = (char*)malloc(sizeof(char)*(strlen(USER_OPTIONS_TXT) + 1));
		myView.type = userView;
		strcpy(myView.text, USER_OPTIONS_TXT);
		break;
	}
	return myView;
}

void popFromMenuStack(MenuStackController* menuStack) {
	if (menuStack->tail) {
		menuStack->tail = menuStack->tail->previous;
		if (menuStack->tail) {
			free(menuStack->tail->next->view.text);
			free(menuStack->tail->next);
			menuStack->tail->next = NULL;
		}
		else {
			free(menuStack->head);
			menuStack->head = NULL;
		}
	}
}

bool logIn(MenuView loginView) {
	ifstream users;
	bool logged = false;
	int ch;
	users.open("users.txt", ios::in);
	system("cls");
	cout << loginView.text;
	if (!users) {
		throw new exception("Could not open file !");
	}
	else if (users.peek() == std::ifstream::traits_type::eof()){
		cout << "There are no users registered for using this application. Please create one !";
		users.close();
	}
	else {
		cout << "\n\t Username: ";
		char usrBuffer[50];
		char pssBuffer[50];
		char inputBuffer[50];
		cin.get(inputBuffer,50);

		while (!users.eof() && logged == false) {
			users >> usrBuffer >> pssBuffer;
			if (strcmp(usrBuffer, inputBuffer) == 0) {
				int attempts = 0;
				cout << "\n\t Password: ";
				while (attempts < 5 && logged == false) {
					char passVerBuffer[50];
					while ((ch = cin.get()) != '\n' && ch != EOF);
					cin.get(passVerBuffer,50);
					if (strcmp(passVerBuffer, pssBuffer) == 0) {
						logged = true;
					}
					else {
						cout << "\n Invalid Password ! Please retry ! Attempts remaining " << 5 - attempts <<".";
						attempts++;
					}
				}
			}
		}
		users.close();
	}
	return logged;
}

bool createNewUser(MenuView createView) {
	system("cls");
	char ch;
	cout << createView.text;
	fstream users;
	bool created = false;
	users.open("users.txt", ios::in|ios::out);
	if (!users) {
		throw new exception("Could not open file !");
	}
	else {
		char inputBuffer[50];
		char pssInputBuffer[50];
		cout << "\n\tPlease input a username: ";
		cin.get(inputBuffer, 50);
		if (users.peek() == std::ifstream::traits_type::eof()) {
			users.seekg(0);
			cout << "\n\tPlease input a password: ";
			while ((ch = cin.get()) != '\n' && ch != EOF);
			cin.get(pssInputBuffer, 50);
			users << inputBuffer <<" "<< pssInputBuffer;
			users << endl;
			created = true;
		}
		else {
			users.seekg(0);
			bool collision = false;
			while (collision == false && !users.eof()) {
				char fileUsr[50];
				char filePass[50];
				users >> fileUsr >> filePass;
				if (strcmp(fileUsr, inputBuffer) == 0) {
					collision = true;
				}
			}
			if (collision == true) {
				cout << "Invalid Username ! An user with that name already exists !";
				Sleep(500);
			}
			else {
				users.close();
				users.open("users.txt", ios::in | ios::out | ios::ate);
				cout << "\n\tPlease input a password: ";
				while ((ch = cin.get()) != '\n' && ch != EOF);
				cin.get(pssInputBuffer, 50);
				users << inputBuffer << " " << pssInputBuffer;
				users << endl;
				created = true;
			}
		}
		users.close();
	}
	return created;
}

bool loginAsAdmin() {
	system("cls");
	bool worked = false;
	cout << "Please input admin password: ";
	char adminPassInput[50];
	cin.get(adminPassInput, 50);
	if (strcmp(adminPassInput, ADMIN_PASS) == 0) {
		worked = true;
	}
	else {
		cout << "\n Invalid Admin Password";
		Sleep(2000);
	}
	return worked;
}

int reconstructInt(char optionsPrinter[4]) {
	int p = 1;
	int number = 0;
	for (int i = strlen(optionsPrinter) - 1; i >= 0; i--) {
		number = (optionsPrinter[i] - '0')*p + number;
		p = p * 10;
	}
	return number;
}

void runApplication(MenuStackController menuStack, AppValues values, HashTableColors hashTableColors, HashTablePaper hashTablePaper, BinaryTreeNode* taskTreeRoot, PrepareToPrintHeap priorityHeap, CircularListNode* startingNode, MainGraphListNode* listStart) {
	bool exitApp = false;
	while (!exitApp) {
		int option;
		if(menuStack.tail->view.type != loginView && menuStack.tail->view.type != createNewView)
			option = displayCurrentMenu(menuStack.tail);
		else if (menuStack.tail->view.type == loginView) {
			try {
				bool logged = logIn(menuStack.tail->view);
				if (logged == true) {
					menuStack = addMenu(menuStack, dynamicMenuCreation(userView));
					option = displayCurrentMenu(menuStack.tail);
				}
				else {
					popFromMenuStack(&menuStack);
					option = displayCurrentMenu(menuStack.tail);
				}
			}
			catch(exception e){
				cout << e.what();
				popFromMenuStack(&menuStack);
				option = displayCurrentMenu(menuStack.tail);
			}
		}
		else if (menuStack.tail->view.type == createNewView) {
			try {
				bool created = createNewUser(menuStack.tail->view);
				if (created == false) {
					popFromMenuStack(&menuStack);
					option = displayCurrentMenu(menuStack.tail);
				}
				else {
					menuStack = addMenu(menuStack, dynamicMenuCreation(userView));
					option = displayCurrentMenu(menuStack.tail);
				}
			}
			catch (exception e) {
				cout << e.what();
				popFromMenuStack(&menuStack);
				option = displayCurrentMenu(menuStack.tail);
			}
		}
		switch (menuStack.tail->view.type)
		{
		case initialView:
			bool worked;
			switch (option) {
			case 1:
				menuStack = addMenu(menuStack, dynamicMenuCreation(loginView));
				break;
			case 2:
				menuStack = addMenu(menuStack, dynamicMenuCreation(createNewView));
				break;
			case 3:
				worked = loginAsAdmin();
				if (worked) {
					menuStack = addMenu(menuStack, dynamicMenuCreation(adminOptionsView));
				}
				break;
			case 4:
				exitApp = true;
				break;
			}
			break;
		case userView:
			switch (option)
			{
			case 1:
				Task newPrinterTask = createTaskFromConsole(hashTableColors, hashTablePaper);
				if (newPrinterTask.taskId == -1) {
					Sleep(2000);
				}
				else {
					++priorityHeap.length;
					taskTreeRoot = addTaskToTree(taskTreeRoot,newPrinterTask);
				}
				
				break;
			case 2:
				cout << "\n\tIn order to use this application, please request a new printer task to be made. \n\tContact the admin in order to print the task, on the appropriate printer, based on their priority. \n\tThe admin can choose the method of printing.";
				Sleep(2000);
				break;
			case 3:
				cout << "Exiting";
				Sleep(500);
				popFromMenuStack(&menuStack);
				popFromMenuStack(&menuStack);
				break;
			}
			break;
		case adminOptionsView:
			
			priorityHeap = insertTasks(taskTreeRoot, priorityHeap);

			switch (option) {
			case 1:
				cout << "\n\tPlease input a priority for the new printer: \n\t 1) Low 2) Medium Low 3) Medium 4) Medium High 5) High\n\t Option: ";
				int priority;
				cin >> priority;
				switch (priority) {
				case 1:
					startingNode = insertPrinterToList(startingNode, low);
					cout << "\n\tInserted printer into circular list !";
					listStart = insertPrinterToMainList(listStart, low);
					cout << "\n\tInserted printer into the graph !";
					listStart = addLinks(listStart, low);
					resetGraphVisited(listStart);
					saveNewPrinterToFile(low);
					break;
				case 2:
					startingNode = insertPrinterToList(startingNode, medium_low);
					cout << "\n\tInserted printer into circular list !";
					listStart = insertPrinterToMainList(listStart, medium_low);
					cout << "\n\tInserted printer into the graph !";
					listStart = addLinks(listStart, medium_low);
					resetGraphVisited(listStart);
					saveNewPrinterToFile(medium_low);
					break;
				case 3:
					startingNode = insertPrinterToList(startingNode, medium);
					cout << "\n\tInserted printer into circular list !";
					listStart = insertPrinterToMainList(listStart, medium);
					cout << "\n\tInserted printer into the graph !";
					listStart = addLinks(listStart, medium);
					resetGraphVisited(listStart);
					saveNewPrinterToFile(medium);
					break;
				case 4:
					startingNode = insertPrinterToList(startingNode, medium_high);
					cout << "\n\tInserted printer into circular list !";
					listStart = insertPrinterToMainList(listStart, medium_high);
					cout << "\n\tInserted printer into the graph !";
					listStart = addLinks(listStart, medium_high);
					resetGraphVisited(listStart);
					saveNewPrinterToFile(medium_high);
					break;
				case 5:
					startingNode = insertPrinterToList(startingNode, high);
					cout << "\n\tInserted printer into circular list !";
					listStart = insertPrinterToMainList(listStart, high);
					cout << "\n\tInserted printer into the graph !";
					listStart = addLinks(listStart, high);
					resetGraphVisited(listStart);
					saveNewPrinterToFile(high);
					break;
				default:
					cout << "\n\tAdding printer failed. Wrong input.";
					Sleep(1000);
				}
				break;
			case 2:
				hashTableColors = addColorFromConsole(hashTableColors);
				break;
			case 3:
				hashTablePaper = addPaperFromConsole(hashTablePaper,values);
				Sleep(500);
				break;
			case 4:
				if (priorityHeap.length > 0) {
					bool found = false;
					Task task = extractPriorityTask(priorityHeap);
					taskTreeRoot = deleteEntry(taskTreeRoot, task.taskId);
					cout << "\n\t Task with id " << task.taskId << " has a priority " << task.taskPriority;
					cout << "\n\t Please choose a printer to print the task, from the following one: " << endl;
					char* options = NULL;
					options	= printToConsolePrintersForPriority(options,task.taskPriority, listStart, found);
					if (found) {
						cout << "\n\t Please select a printer : ";
						char optionPrinter[4];
						cin.getline(optionPrinter,4);
						char* stringFinder = strstr(options, optionPrinter);
						if(stringFinder) {
							bool done = false;
							while (stringFinder && done == false) {
								if ((strchr(stringFinder + strlen(optionPrinter), ' ') == stringFinder + strlen(optionPrinter) && strchr(stringFinder - 1, ' ') == stringFinder - 1) || strlen(options)==strlen(stringFinder)) {
									int code = reconstructInt(optionPrinter);
									printUsingGraph(listStart, code, task);
									done = true;
									resetGraphVisited(listStart);
								}
								else {
									stringFinder = strstr(stringFinder, optionPrinter);
								}
							}
							if (done == false) {
								cout << "\n\t Invalid printer id ! Printing canceled !";
								Sleep(1000);
							}
						}
						else {
							cout << "\n\t Invalid printer id ! Printing canceled !";
							Sleep(1000);
						}
					}
					else {
						cout << "\n\t There are no printers for the priority ! Printing canceled ! Add one.";
						Sleep(1000);
					}
					free(options);
				}
				else {
					cout << "\n\tNo task to print !";
					Sleep(1000);
				}
				break;
			case 5:
				if (priorityHeap.length > 0) {
					while (priorityHeap.length != 0) {
						Task task = extractPriorityTask(priorityHeap);
						bool foundPrinter = attributeTaskCircular(task, startingNode);
						if (foundPrinter) {
							taskTreeRoot = deleteEntry(taskTreeRoot, task.taskId);
						}
						else {
							Sleep(300);
						}
					}
					printCircular(startingNode);
				}
				else {
					cout << "\n\tNo task to print !";
					Sleep(1000);
				}
				break;
			case 6:
				cout << "\n\tPlease input new admin password: ";
				char buffer[50];
				cin.getline(buffer, 50);
				ADMIN_PASS = (char*)malloc(sizeof(char)*(strlen(buffer) + 1));
				strcpy(ADMIN_PASS, buffer);
				break;
			case 7:
				cout << "Exiting";
				Sleep(1000);
				popFromMenuStack(&menuStack);
				break;
			}
			break;
		default: break;
		}
	}
}

void startApplication() {
	MenuStackController menuStack;
	menuStack.head = NULL;
	menuStack.tail = NULL;
	menuStack = addMenu(menuStack, dynamicMenuCreation(initialView));
	AppValues appValues;
	appValues.paperSizes = 6;
	appValues.minPaperHeigh = 37;
	appValues.minPaperWidth = 26;
	appValues.maxPaperWidth = 1682;
	appValues.maxPaperHeigh = 2378;
	HashTableColors hashTableColors = initialize(appValues.palletSize);
	HashTablePaper hashTablePapers = initializeHashTablePaper(appValues.paperSizes);
	BinaryTreeNode* taskTreeRoot = NULL;
	PrepareToPrintHeap priorityHeap;
	priorityHeap.arrayOfTasks = NULL;
	priorityHeap.length = 0;
	priorityHeap.oldLength = 0;
	hashTablePapers = initFromFile(hashTablePapers, appValues);
	hashTableColors = reinstanceFromSave(hashTableColors);
	CircularListNode* startingNode = NULL;
	bool worked;
	MainGraphListNode* graphStart = NULL;
	graphStart = initFromFile(graphStart,worked);
	if (worked == false) {
		cout << "Something went wrong while creating the graph !";
	}
	initPrintersFromFile(startingNode);
 	runApplication(menuStack,appValues,hashTableColors,hashTablePapers, taskTreeRoot, priorityHeap, startingNode, graphStart);
	saveColorsToFile(hashTableColors);
	saveToFile(hashTablePapers);
	hashTablePapers = deleteHashTable(hashTablePapers);
	hashTableColors = deleteHashTableColors(hashTableColors);
	deleteCircularList(startingNode);
	taskTreeRoot = deleteBinaryTree(taskTreeRoot);
	graphStart = deleteGraphMain(graphStart);
	cout << "Thank you for using the app, see you again later !\n";
}


#pragma endregion

void main() {
	startApplication();
}