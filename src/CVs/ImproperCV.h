#pragma once 

#include "CollectiveVariable.h"

#include <array>
#include <math.h>

namespace SSAGES
{
	// Collective variable on an atom position. This will 
	// return the distance of an atom from a particular point 
	// in (1,2,3)-dimensional space.
	class ImproperCV : public CollectiveVariable
	{
	private:
		// IDs of atoms of interest.
		int _atomid1;
		int _atomid2; 
		int _atomid3;
		int _atomid4; 

		// Point in space.
		//Vector3 _position1, Vector3 _position2, Vector3 _position3, Vector3 _position4;

		// Current value of the CV.
		double _val;

		// Constraints in x,y,z dimensions.
		//bool _fixx, _fixy, _fixz;

		// Gradients of the Dihedral CV, dtheta/dri, dtheta/drj, dtheta/drk, dtheta/drl.
		std::vector<Vector3> _grad;

		// Bounds on CV.
		std::array<double, 2> _bounds;

		// Helper function to compute the norm of a vector.
		double norm(const Vector3& v)
		{
			return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		}

		double DotProduct(const Vector3& v, Vector3& w)
		{
			return (v[0]*w[0] + v[1]*w[1] + v[2]*w[2]);
		}

		Vector3 CrossProduct(const Vector3& u, const Vector3& v)
		{
			Vector3 Cross;
			Cross[0] = u[1]*v[2] - u[2]*v[1];
			Cross[1] = u[2]*v[0] - u[0]*v[2];
			Cross[2] = u[0]*v[1] - u[1]*v[0];

			return Cross;
		}

	public:
		// Construct an dihedral CV. The atomids specify 
		// the IDs of the atoms of interest
		// TODO: bounds needs to be an input and periodic boundary conditions
		ImproperCV(int atomid1, int atomid2, int atomid3, int atomid4) : 
		_atomid1(atomid1), _atomid2(atomid2), _atomid3(atomid3), _atomid4(atomid4), _val(0), _grad(0), _bounds{{0,0}}
		{
		}

		// Initialize necessary variables.
		void Initialize(const Snapshot& snapshot) override
		{
			// Initialize gradient. 
			auto n = snapshot.GetPositions().size();		
			_grad.resize(n);
		}

		// Evaluate the CV.
		void Evaluate(const Snapshot& snapshot) override
		{
			// Gradient and value. 
			const auto& pos = snapshot.GetPositions(); 
			const auto& ids = snapshot.GetAtomIDs();


			double ix = 0;
			double iy = 0;
			double iz = 0;
			double jx = 0;
			double jy = 0;
			double jz = 0;
			double kx = 0;
			double ky = 0;
			double kz = 0;
			double lx = 0;
			double ly = 0;
			double lz = 0;
			
			// Loop through atom positions
			for(size_t i = 0; i < pos.size(); ++i)
			{
				_grad[i][0] = 0;
				_grad[i][1] = 0;
				_grad[i][2] = 0;
				// If we are at the atom ID of interest, grab coordinates
				if(ids[i] == _atomid1)
				{
					//coordinates for atom i
					ix = pos[i][0];
					iy = pos[i][1];
					iz = pos[i][2];
				}
				if(ids[i] == _atomid2)
				{
					//coordinates for atom j
					jx = pos[i][0];
					jy = pos[i][1];
					jz = pos[i][2];
				}
				if(ids[i] == _atomid3)
				{
					//coordinates for atom k
					kx = pos[i][0];
					ky = pos[i][1];
					kz = pos[i][2];
				}
				if(ids[i] == _atomid4)
				{
					//coordinates for atom l
					lx = pos[i][0];
					ly = pos[i][1];
					lz = pos[i][2];
				}
			}
			//Calculate pertinent vectors
			Vector3 rij{{
				ix - jx,
				iy - jy,
				iz - jz}};
			Vector3 rkj{{
				kx - jx,
				ky - jy,
				kz - jz}};
			Vector3 rkl;
			rkl[0] = kx - lx;
			rkl[1] = ky - ly; 
			rkl[2] = kz - lz;
			//Calculate dihedral angle

			auto rmj = CrossProduct(rij,rkj);
			auto rnk = CrossProduct(rkj,rkl);
			auto normkj = norm(rkj);
			auto normmj = norm(rmj);
			auto normnk = norm(rnk);
			Vector3 rijrkjprod;
			for(size_t i = 0; i < rijrkjprod.size();i++)
				rijrkjprod[i] = rij[i]*normkj;
			auto rkjrklcross = CrossProduct(rkj, rkl);
			auto y = DotProduct(rijrkjprod, rkjrklcross);
			auto rijrkjcross = CrossProduct(rij, rkj);
			auto x = DotProduct(rijrkjcross, rkjrklcross);
			auto theta = atan2(y, x);
			_val = theta;

			std::cout<<_val<<std::endl;
			// std::cout<<ix<<" "<<iy<<" "<<iz<<std::endl;
			// std::cout<<jx<<" "<<jy<<" "<<jz<<std::endl;
			// std::cout<<kx<<" "<<ky<<" "<<kz<<std::endl;
			// std::cout<<lx<<" "<<ly<<" "<<lz<<std::endl;

			//Calculate gradient
			//atom i
			_grad[_atomid1][0] = normkj/(normmj*normmj)*rmj[0];
			_grad[_atomid1][1] = normkj/(normmj*normmj)*rmj[1];
			_grad[_atomid1][2] = normkj/(normmj*normmj)*rmj[2];

			//atom l
			_grad[_atomid4][0] = normkj/(normnk*normnk)*rnk[0];
			_grad[_atomid4][1] = normkj/(normnk*normnk)*rnk[1];
			_grad[_atomid4][2] = normkj/(normnk*normnk)*rnk[2];

			//atom j
			auto rkldotrkj = DotProduct(rkl, rkj);
			auto rijdotrkj = DotProduct(rij, rkj);
			_grad[_atomid2][0] = (rijdotrkj/(normkj*normkj))*_grad[_atomid1][0]-rkldotrkj/(normkj*normkj)*_grad[_atomid4][0];
			_grad[_atomid2][1] = (rijdotrkj/(normkj*normkj))*_grad[_atomid1][1]-rkldotrkj/(normkj*normkj)*_grad[_atomid4][1];
			_grad[_atomid2][2] = (rijdotrkj/(normkj*normkj))*_grad[_atomid1][2]-rkldotrkj/(normkj*normkj)*_grad[_atomid4][2];

			//atom k
			_grad[_atomid3][0] = (rkldotrkj/(normkj*normkj)-1)*_grad[_atomid4][0]-rijdotrkj/(normkj*normkj)*_grad[_atomid1][0];
			_grad[_atomid3][1] = (rkldotrkj/(normkj*normkj)-1)*_grad[_atomid4][1]-rijdotrkj/(normkj*normkj)*_grad[_atomid1][1];
			_grad[_atomid3][2] = (rkldotrkj/(normkj*normkj)-1)*_grad[_atomid4][2]-rijdotrkj/(normkj*normkj)*_grad[_atomid1][2];

		}

		// Return the value of the CV.
		double GetValue() const override 
		{ 
			return _val; 
		}

		double GetPeriodicValue(double Location) const override
		{
			int n = (int)(Location/(2.0*M_PI()));
			double PeriodicLocation = Location-2.0*n*M_PI();

			PeriodicLocation = Location - n*M_PI()
			if(PeriodicLocation < -M_PI())
				PeriodicLocation += 2.0*M_PI();
			else if (Location > M_PI())
				PeriodicLocation -= 2.0*M_PI();

			return PeriodicLocation;
		}

		// Return the gradient of the CV.
		const std::vector<Vector3>& GetGradient() const override
		{
			return _grad;
		}

		// Return the boundaries of the CV.
		const std::array<double, 2>& GetBoundaries() const override
		{
			return _bounds;
		}

		double GetDifference(const double Location) const override
		{
			double PeriodicDiff = _val - Location;
			PeriodicDiff = GetPeriodicValue(PeriodicDiff);

			if(PeriodicDiff > M_PI())
				PeriodicDiff -= 2.0*M_PI();
			else if(PeriodicDiff < M_PI())
				PeriodicDiff += 2.0*M_PI();
			return PeriodicDiff;
		}
	};
}