/**************************************************************************
Program:   ZXH PatchExtraction Software
Author: lei li
Module:    ......   $
Aim:    for traning   $
Language:  C++
Date:      $Date: From  2018-02-05 $
Version:   $Revision: v 1.0 $
**************************************************************************/

#include <string.h>
#include <iostream> 
#include <time.h> 
#include <math.h>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

#include "zxhImageGipl.h" 
#include "PatchExtractByWorldCoordinate.h"
#include "STLreader.h"
#include <vtkExtractEdges.h>

#include <io.h>  
#include <direct.h> //�����ļ���


using namespace std;

vtkSmartPointer<vtkIdList> GetConnectedVertices(vtkSmartPointer<vtkPolyData> mesh, int id);//Ѱ��node������㺯��

int main(int argc, char* argv[])
{
	string mainfold, Datafold, TargetImageName, LABloodPoolGaussianBlurName, MeshWallLabelName, ScarProbImageName, mesh_name;

	mainfold = "J:\\lilei\\SJTU\\2018MedAI_ScarSeg\\Data\\TraningData";
	string  casename = "C02_Post";
	int HalfPatchLength = 8;
	int N = 6;
	int Offset = 4;

	//mainfold = argv[1];
	//string  casename = argv[2];
	//int HalfPatchLength = atoi(argv[3]);//5
	//int N = atoi(argv[4]);//2
	//int Offset = atoi(argv[5]);//shift (-2,2)?


	srand((unsigned)time(NULL));
	int SiglePatchSize = (N * 2 + 1)*(N * 2 + 1)*(HalfPatchLength * 2 + 1);

	string PathName = mainfold + "\\" + casename;
	TargetImageName = PathName + "\\enhanced.nii.gz";
	LABloodPoolGaussianBlurName = PathName + "\\LA_label_GauiisanBlur.nii.gz";
	MeshWallLabelName = PathName + "\\LA_MeshWallLabel_fixed.nii.gz";
	ScarProbImageName = PathName + "\\LA_MeshWall_422_prob_fixed.nii.gz";
	mesh_name = PathName + "\\LA_Mesh.stl";

	//Patch fold
	string PSize = to_string(2 * N + 1);
	string PLength = to_string(2 * HalfPatchLength + 1);
	string sep = "_";
	string PInfo = "Patch_" + PSize + sep + PSize + sep + PLength;
	string PatchFold = PathName + "\\" + PInfo + "\\";


	int ftyp = _access(PatchFold.c_str(), 0);
	if (0 != ftyp)  //if this is not a directory!  
	{
		const char *PatchFoldName = PatchFold.data();
		_mkdir(PatchFoldName);
	}


	string T_SaveFoldScar = PatchFold + "Patch_T_info.txt";
	string T_SaveFoldNorm = PatchFold + "Patch_T_info_norm.txt";
	string N_SaveFold = PatchFold + "Patch_N_info.txt";

	//--------------------load image-----------------------------------------------------------
	zxhImageData SourceImage, LABloodPoolGaussianBlur, MeshWallLabel;
	zxhImageDataT<float> ScarProbImage;
	zxh::OpenImageSafe(&SourceImage, TargetImageName);
	zxh::OpenImageSafe(&LABloodPoolGaussianBlur, LABloodPoolGaussianBlurName);
	zxh::OpenImageSafe(&MeshWallLabel, MeshWallLabelName);
	zxh::OpenImageSafe(&ScarProbImage, ScarProbImageName);

	//--------------------load mesh-----------------------------------------------------------
	STLreader *stlreader = new STLreader(mesh_name);
	vtkSmartPointer<vtkPolyData> LAMesh = stlreader->decimated;	//load in the mesh
	const int iNumOfMeshPoints = LAMesh->GetNumberOfPoints(); //the number of mesh point


	//Ϊ����ȡmesh�����ıߣ��Ȱѱ���ȡ����
	vtkSmartPointer<vtkExtractEdges> extractEdges = vtkSmartPointer<vtkExtractEdges>::New();
	extractEdges->SetInputData(LAMesh);
	extractEdges->Update();
	vtkSmartPointer<vtkPolyData> mesh = extractEdges->GetOutput();
	//-------------------------------------------------------------------------------

	const int * Size = SourceImage.GetImageSize();

	int ScarAndBoundaryNodesNum = 0;
	for (int ptId = 0; ptId < iNumOfMeshPoints; ptId++)
	{
		float MeshNode_P2I_Coor[] = { LAMesh->GetPoint(ptId)[0], LAMesh->GetPoint(ptId)[1], LAMesh->GetPoint(ptId)[2], 0 };
		SourceImage.GetImageInfo()->PhysicalToImage(MeshNode_P2I_Coor);//��������ת��ͼ������
		int scx = zxh::round(MeshNode_P2I_Coor[0]);
		int scy = zxh::round(MeshNode_P2I_Coor[1]);
		int scz = zxh::round(Size[2] - MeshNode_P2I_Coor[2]);
		bool bIsInsideImage = SourceImage.InsideImage(scx, scy, scz, 0); // ����ͼ��߽�ģ������迼�ǣ�Ҳ����˵��Ĭ��Ϊnormal myo
		if (!bIsInsideImage)
		{
			std::cout << "warning: node " << ptId << " not inside image " << scx << "," << "\n"; //-----------------
			continue;
		}

		float ScarProbValue = ScarProbImage.GetPixelGreyscaleClosest(scx, scy, scz, 0);

		// ------------------------------------go over the mesh neighbors------------------------------------
		vtkSmartPointer<vtkIdList> connectedVertices = GetConnectedVertices(mesh, ptId);
		int NumClist = connectedVertices->GetNumberOfIds();
		int cellId = rand() % NumClist;//rand 
		int nNodeId = connectedVertices->GetId(cellId);
		float NMeshNode_P2I_Coor[] = { LAMesh->GetPoint(nNodeId)[0], LAMesh->GetPoint(nNodeId)[1], LAMesh->GetPoint(nNodeId)[2], 0 };
		SourceImage.GetImageInfo()->PhysicalToImage(NMeshNode_P2I_Coor);//��������ת��ͼ������
		int si = zxh::round(NMeshNode_P2I_Coor[0]);
		int sj = zxh::round(NMeshNode_P2I_Coor[1]);
		int sk = zxh::round(Size[2] - NMeshNode_P2I_Coor[2]);
		bool N_bIsInsideImage = SourceImage.InsideImage(si, sj, sk, 0);
		if (!N_bIsInsideImage)
		{
			std::cout << "warning: node not inside image " << si << "," << "\n"; //-----------------
			continue;
		}
		float N_ScarProbValue = ScarProbImage.GetPixelGreyscaleClosest(si, sj, sk, 0);
		if ((ScarProbValue < 0) || (N_ScarProbValue < 0))
			continue;//this node is PV or Maitral valve

		//---------------lei: using float data to calculate dist
		float wCurrNode[] = { MeshNode_P2I_Coor[0], MeshNode_P2I_Coor[1], Size[2] - MeshNode_P2I_Coor[2], 0 },
			wNeiNode[] = { NMeshNode_P2I_Coor[0], NMeshNode_P2I_Coor[1], Size[2] - NMeshNode_P2I_Coor[2], 0 };
		SourceImage.ImageToWorld(wCurrNode);
		SourceImage.ImageToWorld(wNeiNode);
		float currDist = zxh::VectorOP_Distance(wCurrNode, wNeiNode, 3);
		if (currDist > 5)
			continue;


		if (ScarProbValue > 0.1) // || N_ScarProbValue > 0.1 )  first node is scar 
			ScarAndBoundaryNodesNum++;
	}

	if (ScarAndBoundaryNodesNum > 20000)
	{
		ScarAndBoundaryNodesNum = 20000;//����scar node�ĵ����Ŀ��Ҫ����20000����Ϊnii img�����洢����30000
	}

	int TraningNodeNum = 1.5*ScarAndBoundaryNodesNum;//TO DO
	int Num_Scar = ScarAndBoundaryNodesNum; //prob>0.1
	int Num_Normal = 0.5*ScarAndBoundaryNodesNum; //prob <= 0.1 && prob>=0

	//-----------Generate Patch Image with the real patch size ----------------------
	float spacing111[] = { 1, 1, 1, 1 }; // mm
	//����patch library��size
	int T_newsize[] = { SiglePatchSize, TraningNodeNum, 1, 1 };
	int N_newsize[] = { SiglePatchSize, TraningNodeNum, 1, 1 };
	//-------------------------------------------------------------------------------

	int ScarNumIdex = 0;
	int NormalNumIdex = 0;
	int iOutliers = 0;
	//--------------------------------rand mesh node-------------------------------------- 
	ofstream outfile_scar(T_SaveFoldScar, ios::beg);//output
	outfile_scar << TraningNodeNum << "\n";
	ofstream outfile_norm(T_SaveFoldNorm, ios::beg);//output
	outfile_norm << TraningNodeNum << "\n";
	ofstream N_outfile(N_SaveFold, ios::beg);//output  ptId n_cellId
	N_outfile << TraningNodeNum << "\n";

	zxhImageData T_PatchImage1, T_PatchImage2, T_PatchImage3, N_PatchImage1, N_PatchImage2, N_PatchImage3;
	T_PatchImage1.NewImage(2, T_newsize, spacing111, SourceImage.GetImageInfo());
	T_PatchImage2.NewImage(2, T_newsize, spacing111, SourceImage.GetImageInfo());
	T_PatchImage3.NewImage(2, T_newsize, spacing111, SourceImage.GetImageInfo());
	N_PatchImage1.NewImage(2, N_newsize, spacing111, SourceImage.GetImageInfo());
	N_PatchImage2.NewImage(2, N_newsize, spacing111, SourceImage.GetImageInfo());
	N_PatchImage3.NewImage(2, N_newsize, spacing111, SourceImage.GetImageInfo());

	zxhImageData* ImageArray_T1[3] = { &SourceImage, &LABloodPoolGaussianBlur, &T_PatchImage1 };
	zxhImageData* ImageArray_N1[3] = { &SourceImage, &LABloodPoolGaussianBlur, &N_PatchImage1 };
	zxhImageData* ImageArray_T2[3] = { &SourceImage, &LABloodPoolGaussianBlur, &T_PatchImage2 };
	zxhImageData* ImageArray_N2[3] = { &SourceImage, &LABloodPoolGaussianBlur, &N_PatchImage2 };
	zxhImageData* ImageArray_T3[3] = { &SourceImage, &LABloodPoolGaussianBlur, &T_PatchImage3 };
	zxhImageData* ImageArray_N3[3] = { &SourceImage, &LABloodPoolGaussianBlur, &N_PatchImage3 };

	for (int ptId = 0; (ScarNumIdex < Num_Scar) || (NormalNumIdex < Num_Normal); ptId++)
	{
		if ((ScarNumIdex > ScarAndBoundaryNodesNum) || (ptId >= iNumOfMeshPoints))
		{
			ptId = rand() % iNumOfMeshPoints;//rand for normal
		}
		float MeshNode_P2I_Coor[] = { LAMesh->GetPoint(ptId)[0], LAMesh->GetPoint(ptId)[1], LAMesh->GetPoint(ptId)[2], 0 };
		SourceImage.GetImageInfo()->PhysicalToImage(MeshNode_P2I_Coor);//��������ת��ͼ������
		int scx = zxh::round(MeshNode_P2I_Coor[0]);
		int scy = zxh::round(MeshNode_P2I_Coor[1]);
		int scz = zxh::round(Size[2] - MeshNode_P2I_Coor[2]);
		bool bIsInsideImage = SourceImage.InsideImage(scx, scy, scz, 0); // ����ͼ��߽�ģ������迼�ǣ�Ҳ����˵��Ĭ��Ϊnormal myo
		if (!bIsInsideImage)
		{
			std::cout << "warning: node " << ptId << " not inside image " << scx << "," << "\n"; //-----------------
			continue;
		}

		float ScarProbValue = ScarProbImage.GetPixelGreyscaleClosest(scx, scy, scz, 0);
		if (ScarProbValue < 0)//this node is Mitral valve, ignor (notice, PV boundaries are considered as normal myo)
			continue;

		// ------------------------------------go over the mesh neighbors------------------------------------
		vtkSmartPointer<vtkIdList> connectedVertices = GetConnectedVertices(mesh, ptId);
		int NumClist = connectedVertices->GetNumberOfIds();
		int cellId = rand() % NumClist;//rand 
		int nNodeId = connectedVertices->GetId(cellId);
		float NMeshNode_P2I_Coor[] = { LAMesh->GetPoint(nNodeId)[0], LAMesh->GetPoint(nNodeId)[1], LAMesh->GetPoint(nNodeId)[2], 0 };
		SourceImage.GetImageInfo()->PhysicalToImage(NMeshNode_P2I_Coor);//��������ת��ͼ������
		int si = zxh::round(NMeshNode_P2I_Coor[0]);
		int sj = zxh::round(NMeshNode_P2I_Coor[1]);
		int sk = zxh::round(Size[2] - NMeshNode_P2I_Coor[2]);
		bool N_bIsInsideImage = SourceImage.InsideImage(si, sj, sk, 0);
		if (!N_bIsInsideImage)
		{
			std::cout << "warning: node not inside image " << si << "," << "\n"; //-----------------
			continue;
		}
		float N_ScarProbValue = ScarProbImage.GetPixelGreyscaleClosest(si, sj, sk, 0);
		if ((ScarProbValue < 0) || (N_ScarProbValue < 0))
			continue;//this node is PV or Maitral valve

		//---------------lei: using float data to calculate dist
		float wCurrNode[] = { MeshNode_P2I_Coor[0], MeshNode_P2I_Coor[1], Size[2] - MeshNode_P2I_Coor[2], 0 },
			wNeiNode[] = { NMeshNode_P2I_Coor[0], NMeshNode_P2I_Coor[1], Size[2] - NMeshNode_P2I_Coor[2], 0 };
		SourceImage.ImageToWorld(wCurrNode);
		SourceImage.ImageToWorld(wNeiNode);
		float currDist = zxh::VectorOP_Distance(wCurrNode, wNeiNode, 3);
		if (currDist > 5)
		{
			iOutliers++;
			//std::cout << "error: dist:" << currDist << " is too large for node " << ptId << "\n";
			continue;
		}

		//Save n-link weight 
		bool bIsScarNode = false;
		if (ScarProbValue > 0.1) // || N_ScarProbValue > 0 )  first node is scar 
			bIsScarNode = true;

		if (ScarNumIdex < Num_Scar && bIsScarNode == false)//scar��û��ȡ�����Ⱦ�ȫ������normal��
			continue;

		if (ScarNumIdex == Num_Scar && bIsScarNode == true)
			continue;
		if (ScarNumIdex == Num_Normal && bIsScarNode == false)
			continue;


		//---------------lei: using float data to extract patch
		float InputWorldCoordRand[] = { MeshNode_P2I_Coor[0], MeshNode_P2I_Coor[1], Size[2] - MeshNode_P2I_Coor[2], 0 };
		SourceImage.GetImageInfo()->ImageToWorld(InputWorldCoordRand);
		float N_InputWorldCoordRand[] = { NMeshNode_P2I_Coor[0], NMeshNode_P2I_Coor[1], Size[2] - NMeshNode_P2I_Coor[2], 0 };
		SourceImage.GetImageInfo()->ImageToWorld(N_InputWorldCoordRand);

		//------------------------------------Generate neighboor node patch-----------------------------------------------------	
		int PatchInfo[4] = { N, HalfPatchLength, 0, 0 };//kk��֤һ��

		float randoffset = float((rand() % (200 * Offset)) - 100 * Offset) / 100.0;//rand shift	
		if (PatchExtractByWorldCoordinate::GeneratePatchExtractByWorldWithRandOffset(InputWorldCoordRand, PatchInfo, randoffset, ImageArray_T1, ScarNumIdex + NormalNumIdex, 1.0) == false)
			continue;//for t-link
		if (PatchExtractByWorldCoordinate::GeneratePatchExtractByWorldWithRandOffset(N_InputWorldCoordRand, PatchInfo, randoffset, ImageArray_N1, ScarNumIdex + NormalNumIdex, 1.0) == false)
			continue;;//for n-link

		if (PatchExtractByWorldCoordinate::GeneratePatchExtractByWorldWithRandOffset(InputWorldCoordRand, PatchInfo, randoffset, ImageArray_T2, ScarNumIdex + NormalNumIdex, 2.0) == false)
			continue;//for t-link
		if (PatchExtractByWorldCoordinate::GeneratePatchExtractByWorldWithRandOffset(N_InputWorldCoordRand, PatchInfo, randoffset, ImageArray_N2, ScarNumIdex + NormalNumIdex, 2.0) == false)
			continue;;//for n-link

		if (PatchExtractByWorldCoordinate::GeneratePatchExtractByWorldWithRandOffset(InputWorldCoordRand, PatchInfo, randoffset, ImageArray_T3, ScarNumIdex + NormalNumIdex, 3.0) == false)
			continue;//for t-link
		if (PatchExtractByWorldCoordinate::GeneratePatchExtractByWorldWithRandOffset(N_InputWorldCoordRand, PatchInfo, randoffset, ImageArray_N3, ScarNumIdex + NormalNumIdex, 3.0) == false)
			continue;;//for n-link

		float nlink = (1 - ScarProbValue)*(1 - N_ScarProbValue) + ScarProbValue*N_ScarProbValue;
		currDist = zxh::VectorOP_Distance(InputWorldCoordRand, N_InputWorldCoordRand, 3);
		if (currDist < ZXH_FloatInfinitesimal)
		{
		std:cerr << "error: distance == 0 \n";
			currDist = 0.1;
		}

		outfile_scar << ptId << " " << ScarProbValue << "\n";
		outfile_norm << ptId << " " << 1.0 - ScarProbValue << "\n";
		N_outfile << "\t" << ptId << " " << cellId << " " << currDist << "\n";
		//N_outfile << ptId << " " << cellId << " " << currDist << "\n"; 
		N_outfile << ptId << " " << cellId << " " << nlink << "\n";

		if (bIsScarNode == true)
			ScarNumIdex++;
		else //if ( bIsScarNode==false )
			NormalNumIdex++;

	}

	outfile_scar.close();
	outfile_norm.close();
	N_outfile.close();
	//-----------------------save patch with PatchInfo--------------------


	zxh::SaveImage(&T_PatchImage1, PatchFold + "Patch_T_scale1.nii.gz");
	zxh::SaveImage(&T_PatchImage1, PatchFold + "Patch_N1_scale1.nii.gz");
	zxh::SaveImage(&N_PatchImage1, PatchFold + "Patch_N2_scale1.nii.gz");

	zxh::SaveImage(&T_PatchImage1, PatchFold + "Patch_T_scale2.nii.gz");
	zxh::SaveImage(&T_PatchImage1, PatchFold + "Patch_N1_scale2.nii.gz");
	zxh::SaveImage(&N_PatchImage1, PatchFold + "Patch_N2_scale2.nii.gz");

	zxh::SaveImage(&T_PatchImage1, PatchFold + "Patch_T_scale3.nii.gz");
	zxh::SaveImage(&T_PatchImage1, PatchFold + "Patch_N1_scale3.nii.gz");
	zxh::SaveImage(&N_PatchImage1, PatchFold + "Patch_N2_scale3.nii.gz");


return 0;
}


vtkSmartPointer<vtkIdList> GetConnectedVertices(vtkSmartPointer<vtkPolyData> mesh, int id)
{
	vtkSmartPointer<vtkIdList> connectedVertices =
		vtkSmartPointer<vtkIdList>::New();

	//get all cells that vertex 'id' is a part of
	vtkSmartPointer<vtkIdList> cellIdList =
		vtkSmartPointer<vtkIdList>::New();
	mesh->GetPointCells(id, cellIdList);

	for (vtkIdType i = 0; i < cellIdList->GetNumberOfIds(); i++)
	{

		vtkSmartPointer<vtkIdList> pointIdList =
			vtkSmartPointer<vtkIdList>::New();
		mesh->GetCellPoints(cellIdList->GetId(i), pointIdList);

		if (pointIdList->GetId(0) != id)
		{

			connectedVertices->InsertNextId(pointIdList->GetId(0));
		}
		else
		{
			connectedVertices->InsertNextId(pointIdList->GetId(1));
		}
	}

	return connectedVertices;
}
