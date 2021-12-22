#ifndef TRANSIT_H
#define TRANSIT_H

#include "place.h"

class Place;

/***************************************************** 
 * class: Transit 
 * 
 * Defines and stores attributes of a single transit 
 * mode object 
 * 
 *****************************************************/

class Transit : public Place{
public:

	//
	// Constructors
	//

	/**
	 * \brief Creates a Transit object with default attributes
	 */
	Transit() = default;

	/**
	 * \brief Creates a Transit object 
	 * \details Transit with custom ID, type, and infection parameters
	 *
	 * @param transit_ID - ID of the transit object 
	 * @param severity_cor - severity correction for symptomatic
	 * @param beta - infection transmission rate, 1/time
	 * @param tr_type - transit type
	 */
	Transit(const int transit_ID, const double beta,
				const double severity_cor, const std::string tr_type) : 
			type(tr_type), Place(transit_ID, 0.0, 0.0, severity_cor, beta){ }

	//
 	// I/O
	//

	/**
	 * \brief Save information about a Transit object
	 * \details Saves to a file, everything but detailed agent 
	 * 		information; order is ID | x | y | number of agents | 
	 * 		number of infected agents | ck | beta_j | type 
	 * 		Delimiter is a space.
	 * 	@param where - output stream
	 */
	void print_basic(std::ostream& where) const override;

private:
	// Transit type 
	std::string type;
};
#endif
