#ifndef CYCLASS_SRC_FUEL_FAB_H_
#define CYCLASS_SRC_FUEL_FAB_H_



#include "CLASSAdaptator.h"

#include <string>
#include "cyclus.h"

namespace cyclass {

    /// FuelFab takes in 2 streams of material and mixes them in ratios in order to
    /// supply material that matches some neutronics properties of reqeusted
    /// material.  It uses an equivalence type method [1] inspired by a similar
    /// approach in the CLASS fuel cycle simulator.

    /// The major part of this Fuel_fuel comes from the CYCAMORE::FuelFab, where
    /// mainly FuelFab::GetMatlBids and FuelFab::GetMatlTrades have been updated.
    ///
    /// The neural network used, allows to predict the enrichment of fissil material
    /// requiert to be able to reach a cetain burups with the fuel. This Model is
    /// capable to deal only plutnoium fissil stream (238Pu+239Pu+240Pu+2341Pu+242Pu+
    /// 241Am -- the 241Am come from the decay of the 241Pu) and uranium based fill
    /// stream (235U+238U)...

    /// @code
    /// [1]  B. Leniau, B. Mouginot, N. Thiolliere et. al. "A neural network approach
    /// for burn-up calculation and its application to the dynamic fuel cycle code CLASS."
    /// @endcode
    class FuelFab : public cyclus::Facility {
#pragma cyclus note { \
"niche": "fabrication", \
"doc": \
"FuelFab takes in 2 streams of material and mixes them in ratios in order to" \
"", \
}
    public:
        FuelFab(cyclus::Context* ctx);
        virtual ~FuelFab(){};


#pragma cyclus

        virtual void Tick(){};
        virtual void Tock(){};
        virtual void EnterNotify();

        virtual std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr> GetMatlBids(
                                                                                  cyclus::CommodMap<cyclus::Material>::type& commod_requests);

        virtual void GetMatlTrades(
                                   const std::vector<cyclus::Trade<cyclus::Material> >& trades,
                                   std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                                   cyclus::Material::Ptr> >& responses);

        virtual void AcceptMatlTrades(const std::vector<std::pair<
                                      cyclus::Trade<cyclus::Material>, cyclus::Material::Ptr> >& responses);

        virtual std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr>
        GetMatlRequests();

    private:

        CLASSAdaptator* MyCLASSAdaptator;


#pragma cyclus var { \
"doc": "Ordered list of commodities on which to requesting filler stream material.", \
"uilabel": "Filler Stream Commodities", \
"uitype": ["oneormore", "incommodity"], \
}
        std::vector<std::string> fill_commods;
#pragma cyclus var { \
"default": [], \
"uilabel": "Filler Stream Preferences", \
"doc": "Filler stream commodity request preferences for each of the given filler commodities (same order)." \
" If unspecified, default is to use 1.0 for all preferences.", \
}
        std::vector<double> fill_commod_prefs;
#pragma cyclus var { \
"doc": "Name of recipe to be used in filler material stream requests.", \
"uilabel": "Filler Stream Recipe", \
"uitype": "recipe", \
}
        std::string fill_recipe;
#pragma cyclus var { \
"doc": "Size of filler material stream inventory.", \
"uilabel": "Filler Stream Inventory Capacity", \
"units": "kg", \
}
        double fill_size;
#pragma cyclus var {"capacity": "fill_size"}
        cyclus::toolkit::ResBuf<cyclus::Material> fill;

#pragma cyclus var { \
"doc": "Ordered list of commodities on which to requesting fissile stream material.", \
"uilabel": "Fissile Stream Commodities", \
"uitype": ["oneormore", "incommodity"], \
}
        std::vector<std::string> fiss_commods;
#pragma cyclus var { \
"default": [], \
"uilabel": "Fissile Stream Preferences", \
"doc": "Fissile stream commodity request preferences for each of the given fissile commodities (same order)." \
" If unspecified, default is to use 1.0 for all preferences.", \
}
        std::vector<double> fiss_commod_prefs;
#pragma cyclus var { \
"doc": "Name for recipe to be used in fissile stream requests." \
" Empty string results in use of an empty dummy recipe.", \
"uitype": "recipe", \
"uilabel": "Fissile Stream Recipe", \
"default": "", \
}
        std::string fiss_recipe;
      ///////// Model params ///////////



#pragma cyclus var { \
"default": "eq", \
"doc": "Equivalence Model name.",	\
"uilabel": "Name of the equivalence Model", \
}
      std::string eq_model;
#pragma cyclus var { \
"doc": "equivalence Parameter line",	\
"uilabel": "Parameter line for the equivalence model", \
}
      std::string eq_command;



#pragma cyclus var { \
"doc": "Size of fissile material stream inventory.", \
"uilabel": "Fissile Stream Inventory Capacity", \
"units": "kg", \
}
        double fiss_size;
#pragma cyclus var {"capacity": "fiss_size"}
        cyclus::toolkit::ResBuf<cyclus::Material> fiss;

#pragma cyclus var { \
"doc": "Commodity on which to request material for top-up stream." \
" This MUST be set if 'topup_size > 0'.", \
"uilabel": "Top-up Stream Commodity", \
"default": "", \
"uitype": "incommodity", \
}

#pragma cyclus var { \
"doc": "Commodity on which to offer/supply mixed fuel material.", \
"uilabel": "Output Commodity", \
"uitype": "outcommodity", \
}
        std::string outcommod;

#pragma cyclus var { \
"doc": "Maximum number of kg of fuel material that can be supplied per time step.", \
"uilabel": "Maximum Throughput", \
"units": "kg", \
}
        double throughput;


        // intra-time-step state - no need to be a state var
        // map<request, inventory name>
        std::map<cyclus::Request<cyclus::Material>*, std::string> req_inventories_;
    };

    double AtomToMassFrac(double atomfrac, cyclus::Composition::Ptr c1, cyclus::Composition::Ptr c2);
    
} // namespace cyclass


#endif  // CYCLASS_SRC_FUEL_FAB_H_
