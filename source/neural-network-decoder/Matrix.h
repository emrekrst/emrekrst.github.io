/*
@Filename: Matrix.h
@Author: Ivan Ling (ivan.ling@soton.ac.uk)
@Version: 1.0

@Date: 27/10/2025
@copyright Copyright (c) 2025
*/

#ifndef MATRIX_H
#define MATRIX_H

#include <vector>
#include <iostream>
#include <iomanip>

using namespace std;

class Matrix
{
private:
	int rows;
	int col;

public:
	vector<vector<double>> data;

	Matrix();
	Matrix(vector<vector<double>> d);
	Matrix(int r, int c, bool isRandom);

	int getRows() const;
	int getCol() const;
	vector<vector<double>> getData() const;

	void print_matrix() const;

	Matrix operator*(const Matrix& mat) const;
	Matrix operator+(const Matrix& mat) const;
	Matrix operator-(const Matrix& mat) const;
	Matrix operator*(double num) const;

	Matrix transpose() const;
	Matrix element_multiply(const Matrix& mat) const;
};

#endif // MATRIX_H