/*
@Filename: Matrix.cpp
@Author: Ivan Ling (ivan.ling@soton.ac.uk)
@Version: 1.0

@Date: 27/10/2025
@copyright Copyright (c) 2025
*/

#include "Matrix.h"
#include <cstdlib> // For rand() and RAND_MAX

Matrix::Matrix()
{
	rows = 0;
    col = 0;
}

Matrix::Matrix(vector<vector<double>> d)
{
	if (d.empty()) {
        rows = 0;
        col = 0;
        data = d; 
    } else {
        rows = d.size(); 
        col = d[0].size();  
        data = d;
    }
}

Matrix::Matrix(int r, int c, bool isRandom)
{
	rows = r;
	col = c;

	data.resize(rows);
	for(int i = 0; i < rows; ++i) {
		data[i].resize(col);
	}
	if (isRandom) {
		for(int i=0; i<r; ++i){
			for(int j=0; j < c; ++j) {
				data[i][j]=(double)rand() / RAND_MAX;
			}
		}
	}
}

int Matrix::getRows() const { return rows; }
int Matrix::getCol() const { return col; }
vector<vector<double>> Matrix::getData() const { return data; }

void Matrix::print_matrix() const{
	for (int i = 0; i < rows; ++i){
		for (int j = 0; j < col; ++j){
			std::cout << std::fixed << std::setprecision(4) << std::setw(10) << data[i][j] << " ";
		}
		std::cout << endl;
	}
}

Matrix Matrix::operator*(const Matrix& mat) const
{
	if(this->col != mat.getRows()){
		std::cerr<<"Error: Matrix A columm size must be equal to Matrix B rows for multiplication"<<endl;
		return Matrix();
	}
	int result_rows = this->rows;
	int result_col = mat.getCol();
	vector<vector<double>> result(result_rows, vector<double>(result_col, 0.0));

	for (int i = 0; i < result_rows; ++i) {
		for (int j = 0; j < result_col; ++j) {
			double sum = 0.0;
			for (int k = 0; k < this->col; ++k){
				sum += this->data[i][k]*mat.data[k][j]; 
			}
		result[i][j]=sum;
		}
	}
	return Matrix(result);
}

Matrix Matrix::operator+(const Matrix& mat) const
{
	if (this->rows != mat.getRows() || this->col != mat.getCol()){
		std::cerr<< "Error: Matrices must have the same dimensions for addition"<<endl;
		return Matrix();
	}

	vector<vector<double>> result(rows, vector<double>(col));

	for(int i=0; i<rows; ++i) {
		for(int j = 0; j<col; ++j){
			result[i][j] = this->data[i][j] + mat.data[i][j];
		}
	}

	return Matrix(result);
}

Matrix Matrix::operator-(const Matrix& mat) const
{
	if (this->rows != mat.getRows() || this->col != mat.getCol()){
		std::cerr<< "Error: Matrices must have the same dimensions for substraction"<<endl;
		return Matrix();
	}

	vector<vector<double>> result(rows, vector<double>(col));

	for(int i=0; i<rows; ++i) {
		for(int j = 0; j<col; ++j){
			result[i][j] = this->data[i][j] - mat.data[i][j];
		}
	}

	return Matrix(result);
}

Matrix Matrix::operator*(double num) const
{
	vector<vector<double>> result(rows, vector<double>(col));
	for (int i = 0; i < rows; ++i) {
		for(int j = 0; j < col; ++j) {
			result[i][j] = this->data[i][j] * num;
		}
	}
	return Matrix(result);
}

Matrix Matrix::transpose() const
{
	vector<vector<double>> result_data(col, vector<double>(rows));
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < col; ++j) {
			result_data[j][i] = this->data[i][j];
		}
	}
	return Matrix(result_data);
}

Matrix Matrix::element_multiply(const Matrix& mat) const
{
	if(this->rows != mat.getRows()||this->col != mat.getCol()) {
		std::cerr << "Error: Matrices must have the same dimensions for element-wise multiplication." << endl;
		exit(1);
	}
	vector<vector<double>> result_data(rows, vector<double>(col));
	for(int i = 0; i < rows; ++i){
		for(int j = 0; j < col; ++j){
			result_data[i][j] = this->data[i][j] * mat.data[i][j];
		}
	}
	return Matrix(result_data);
}