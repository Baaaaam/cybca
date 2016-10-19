#include "fuel_fab.h"

#include <sstream>

using cyclus::Material;
using cyclus::Composition;
using pyne::simple_xs;

//#define cyDBGL		std::cout << __FILE__ << " : " << __LINE__ << " [" << __FUNCTION__ << "]" << std::endl;
#define cyDBGL		;

#define SHOW(X)                                                     \
std::cout << std::setprecision(17) << __FILE__ << ":" << __LINE__ \
<< ": " #X " = " << X << "\n"

namespace cyclass {

  class FissConverter : public cyclus::Converter<cyclus::Material> {
  public:
    FissConverter( Composition::Ptr c_fiss, Composition::Ptr c_fill) :
    c_fiss_(c_fiss), c_fill_(c_fill) {}

    virtual ~FissConverter() {}

    virtual double convert(cyclus::Material::Ptr m, cyclus::Arc const* a = NULL,
                           cyclus::ExchangeTranslationContext<
                           cyclus::Material> const* ctx = NULL) const {

      Composition::Ptr fuel = m->comp();
      Composition::Ptr fuel_fissil = ExtractAccordinglist( fuel, c_fiss_);
      Composition::Ptr fuel_fertil = ExtractAccordinglist( fuel, c_fill_);

      if( std::abs(AtomIn(fuel_fertil) + AtomIn(fuel_fissil) - AtomIn(fuel)) >1e-10 ){
        std::cout << "You fuel has nuclei that this model could not manage.."<< std::endl;
        exit(1);
      }
      double enrichment = AtomIn(fuel_fissil)/AtomIn(fuel);
      return AtomToMassFrac(enrichment, c_fiss_, c_fill_) * m->quantity();
    }

  private:
    Composition::Ptr c_fiss_;
    Composition::Ptr c_fill_;

  };

  class FillConverter : public cyclus::Converter<cyclus::Material> {
  public:
    FillConverter( Composition::Ptr c_fill, Composition::Ptr c_fiss) :
    c_fiss_(c_fiss), c_fill_(c_fill) {}

    virtual ~FillConverter() {}

    virtual double convert(cyclus::Material::Ptr m, cyclus::Arc const* a = NULL,
                           cyclus::ExchangeTranslationContext<
                           cyclus::Material> const* ctx = NULL) const {

      Composition::Ptr fuel = m->comp();
      Composition::Ptr fuel_fissil = ExtractAccordinglist( fuel, c_fiss_);
      Composition::Ptr fuel_fertil = ExtractAccordinglist( fuel, c_fill_);

      if( std::abs(AtomIn(fuel_fertil) + AtomIn(fuel_fissil) - AtomIn(fuel)) >1e-10 ){
        std::cout << "You fuel has nuclei that this model could not manage.."<< std::endl;
        exit(1);
      }
      double enrichment = AtomIn(fuel_fissil)/AtomIn(fuel);
      return AtomToMassFrac(1 - enrichment, c_fill_, c_fiss_) * m->quantity();
    }

  private:
    double frac_;
    Composition::Ptr c_fiss_;
    Composition::Ptr c_fill_;

  };


  //________________________________________________________________________
  FuelFab::FuelFab(cyclus::Context* ctx):
  cyclus::Facility(ctx),
  fill_size(0),
  fiss_size(0),
  throughput(0),
  eq_model("eq"),
  eq_command("eq"){
    cyDBGL
    cyclus::Warn<cyclus::EXPERIMENTAL_WARNING>(
                                               "the FuelFab archetype "
                                               "is experimental");
    MyCLASSAdaptator = 0;

    cyDBGL
  }

  void FuelFab::EnterNotify() {
    cyDBGL
    cyclus::Facility::EnterNotify();

    if (fiss_commod_prefs.empty()) {
      for (int i = 0; i < fiss_commods.size(); i++) {
        fiss_commod_prefs.push_back(cyclus::kDefaultPref);
      }
    } else if (fiss_commod_prefs.size() != fiss_commods.size()) {
      std::stringstream ss;
      ss << "prototype '" << prototype() << "' has " << fiss_commod_prefs.size()
      << " fiss_commod_prefs vals, expected " << fiss_commods.size();
      throw cyclus::ValidationError(ss.str());
    }

    if (fill_commod_prefs.empty()) {
      for (int i = 0; i < fill_commods.size(); i++) {
        fill_commod_prefs.push_back(cyclus::kDefaultPref);
      }
    } else if (fill_commod_prefs.size() != fill_commods.size()) {
      std::stringstream ss;
      ss << "prototype '" << prototype() << "' has " << fill_commod_prefs.size()
      << " fill_commod_prefs vals, expected " << fill_commods.size();
      throw cyclus::ValidationError(ss.str());
    }
    cyDBGL
  }

  //________________________________________________________________________
  std::set<cyclus::RequestPortfolio<Material>::Ptr> FuelFab::GetMatlRequests() {
    using cyclus::RequestPortfolio;



    cyDBGL
    std::set< RequestPortfolio<Material>::Ptr > ports;

    bool exclusive = false;

    if (fiss.space() > cyclus::eps()) {
      RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());

      Material::Ptr m = cyclus::NewBlankMaterial(fiss.space());
      if (!fiss_recipe.empty()) {
        Composition::Ptr c = context()->GetRecipe(fiss_recipe);
        m = Material::CreateUntracked(fiss.space(), c);
      }

      std::vector<cyclus::Request<Material>*> reqs;
      for (int i = 0; i < fiss_commods.size(); i++) {
        std::string commod = fiss_commods[i];
        double pref = fiss_commod_prefs[i];
        reqs.push_back(port->AddRequest(m, this, commod, pref, exclusive));
        req_inventories_[reqs.back()] = "fiss";
      }
      port->AddMutualReqs(reqs);
      ports.insert(port);
    }

    if (fill.space() > cyclus::eps()) {
      RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());

      Material::Ptr m = cyclus::NewBlankMaterial(fill.space());
      if (!fill_recipe.empty()) {
        Composition::Ptr c = context()->GetRecipe(fill_recipe);
        m = Material::CreateUntracked(fill.space(), c);
      }

      std::vector<cyclus::Request<Material>*> reqs;
      for (int i = 0; i < fill_commods.size(); i++) {
        std::string commod = fill_commods[i];
        double pref = fill_commod_prefs[i];
        reqs.push_back(port->AddRequest(m, this, commod, pref, exclusive));
        req_inventories_[reqs.back()] = "fill";
      }
      port->AddMutualReqs(reqs);
      ports.insert(port);
    }

    cyDBGL
    return ports;
  }

  //________________________________________________________________________
  bool Contains(std::vector<std::string> vec, std::string s) {
    cyDBGL
    for (int i = 0; i < vec.size(); i++) {
      if (vec[i] == s) {
        return true;
      }
    }
    cyDBGL
    return false;
  }

  //________________________________________________________________________
  void FuelFab::AcceptMatlTrades(const std::vector<
                                 std::pair<cyclus::Trade<Material>,
                                 Material::Ptr> >& responses) {
    cyDBGL
    std::vector< std::pair <cyclus::Trade<cyclus::Material>, cyclus::Material::Ptr> >::const_iterator trade;

    for (trade = responses.begin(); trade != responses.end(); ++trade) {
      std::string commod = trade->first.request->commodity();
      double req_qty = trade->first.request->target()->quantity();
      cyclus::Request<Material>* req = trade->first.request;
      Material::Ptr m = trade->second;
      if (req_inventories_[req] == "fill") {
        fill.Push(m);
      } else if (req_inventories_[req] == "fiss") {
        fiss.Push(m);
      } else {
        throw cyclus::ValueError("cyclass::FuelFab was overmatched on requests");
      }
    }

    req_inventories_.clear();

    // IMPORTANT - each buffer needs to be a single homogenous composition or
    // the inventory mixing constraints for bids don't work
    if (fill.count() > 1) {
      fill.Push(cyclus::toolkit::Squash(fill.PopN(fill.count())));
    }
    if (fiss.count() > 1) {
      fiss.Push(cyclus::toolkit::Squash(fiss.PopN(fiss.count())));
    }
    cyDBGL
  }

  //________________________________________________________________________
  std::set<cyclus::BidPortfolio<Material>::Ptr> FuelFab::GetMatlBids(
                                                                     cyclus::CommodMap<Material>::type& commod_requests) {
    cyDBGL
    using cyclus::BidPortfolio;

    std::set<BidPortfolio<Material>::Ptr> ports;
    std::vector<cyclus::Request<Material>*>& reqs = commod_requests[outcommod];

    if(!MyCLASSAdaptator){
      MyCLASSAdaptator = new CLASSAdaptator(eq_model, eq_command);
    }

    if (throughput == 0) {
      return ports;
    } else if (reqs.size() == 0) {
      return ports;
    }

    Composition::Ptr c_fill;  // no default needed - this is non-optional parameter
    if (fill.count() > 0) {
      c_fill = fill.Peek()->comp();
    } else {
      c_fill = context()->GetRecipe(fill_recipe);
    }

    Composition::Ptr c_fiss;
    if (fiss.count() > 0) {
      c_fiss = fiss.Peek()->comp();
    } else if (!fiss_recipe.empty()) {
      c_fiss = context()->GetRecipe(fiss_recipe);
    } else{
      return ports;
    }


    BidPortfolio<Material>::Ptr port(new BidPortfolio<Material>());
    for (int j = 0; j < reqs.size(); j++) {
      cyDBGL
      cyclus::Request<Material>* req = reqs[j];
      cyDBGL

      Composition::Ptr tgt = req->target()->comp();
      cyDBGL
      double tgt_qty = req->target()->quantity();
      cyDBGL
      double BU_tgt = MyCLASSAdaptator->GetBU(tgt);

      cyDBGL
      double fiss_frac = MyCLASSAdaptator->GetEnrichment(c_fiss, c_fill, BU_tgt);
      cyDBGL


      double fill_frac = 1 - fiss_frac;
      cyDBGL

      fiss_frac = AtomToMassFrac(fiss_frac, c_fiss, c_fill);
      fill_frac = AtomToMassFrac(fill_frac, c_fill, c_fiss);

      Material::Ptr m1 = Material::CreateUntracked(fiss_frac * tgt_qty, c_fiss);
      cyDBGL

      Material::Ptr m2 = Material::CreateUntracked(fill_frac * tgt_qty, c_fill);
      m1->Absorb(m2);
      cyDBGL

      bool exclusive = false;
      port->AddBid(req, m1, this, exclusive);
      cyDBGL
    }

    //cyclus::Converter<Material>::Ptr fissconv( new FissConverter(c_fill, c_fiss) );
    //cyclus::Converter<Material>::Ptr fillconv( new FillConverter(c_fill, c_fiss) );
    // important! - the std::max calls prevent CapacityConstraint throwing a zero
    // cap exception
    //cyclus::CapacityConstraint<Material> fissc(std::max(fiss.quantity(), 1e-10),fissconv);
    //cyclus::CapacityConstraint<Material> fillc(std::max(fill.quantity(), 1e-10),fillconv);
cyclus::CapacityConstraint<Material> fissc(std::max(fiss.quantity(), 1e-10));
    cyclus::CapacityConstraint<Material> fillc(std::max(fill.quantity(), 1e-10));

    port->AddConstraint(fillc);
    port->AddConstraint(fissc);

    cyclus::CapacityConstraint<Material> cc(throughput);
    port->AddConstraint(cc);
    ports.insert(port);
    cyDBGL
    return ports;
  }

  //________________________________________________________________________
  void FuelFab::GetMatlTrades(
                              const std::vector<cyclus::Trade<Material> >& trades,
                              std::vector<std::pair<cyclus::Trade<Material>, Material::Ptr> >&
                              responses) {
    using cyclus::Trade;
    cyDBGL

    if(!MyCLASSAdaptator){
      MyCLASSAdaptator = new CLASSAdaptator(eq_model, eq_command);
    }

    // guard against cases where a buffer is empty - this is okay because some trades
    // may not need that particular buffer.

    std::vector<cyclus::Trade<cyclus::Material> >::const_iterator it;
    double tot = 0;
    for (int i = 0; i < trades.size(); i++) {
      Material::Ptr tgt = trades[i].request->target();

      double qty = trades[i].amt;

      tot += qty;
      if (tot > throughput + cyclus::eps()) {
        std::stringstream ss;
        ss << "FuelFab was matched above throughput limit: " << tot << " > "
        << throughput;
        throw cyclus::ValueError(ss.str());
      }

      if (fiss.count() == 0) {
        // use straight filler to satisfy this request
        double fillqty = qty;
        if (std::abs(fillqty - fill.quantity()) < cyclus::eps()) {
          fillqty = std::min(fill.quantity(), qty);
        }
        responses.push_back(std::make_pair(trades[i], fill.Pop(fillqty,cyclus::eps())));
      } else if (fill.count() == 0 ) {
        // use straight fissile to satisfy this request
        double fissqty = qty;
        if (std::abs(fissqty - fiss.quantity()) < cyclus::eps()) {
          fissqty = std::min(fiss.quantity(), qty);
        }
        responses.push_back(std::make_pair(trades[i], fiss.Pop(fissqty,cyclus::eps())));
      }  else {

        Composition::Ptr c_fiss = fiss.Peek()->comp();
        Composition::Ptr c_fill = fill.Peek()->comp();

        double BU_tgt = MyCLASSAdaptator->GetBU(tgt->comp());

        double fiss_frac = MyCLASSAdaptator->GetEnrichment(c_fiss, c_fill, BU_tgt);
        double fill_frac = 1-fiss_frac;

        //Atomic to mass conversion...
        fiss_frac =
        AtomToMassFrac(fiss_frac, fiss.Peek()->comp(), fill.Peek()->comp());
        fill_frac =
        AtomToMassFrac(fill_frac, fill.Peek()->comp(), fiss.Peek()->comp());

        double fissqty = fiss_frac*qty;
        if (std::abs(fissqty - fiss.quantity()) < cyclus::eps()) {
          fissqty = std::min(fiss.quantity(), fiss_frac*qty);
        }
        double fillqty = fill_frac*qty;
        if (std::abs(fillqty - fill.quantity()) < cyclus::eps()) {
          fillqty = std::min(fill.quantity(), fill_frac*qty);
        }

        Material::Ptr m = fiss.Pop(fissqty,cyclus::eps());
        // this if block prevents zero qty ResBuf pop exceptions
        if (fill_frac > 0) {
          m->Absorb(fill.Pop(fillqty,cyclus::eps()));
        }
        responses.push_back(std::make_pair(trades[i], m));
      }

    }
    cyDBGL
  }

  //________________________________________________________________________
  extern "C" cyclus::Agent* ConstructFuelFab(cyclus::Context* ctx) {
    cyDBGL
    return new FuelFab(ctx);
  }

  // Convert an atom frac (n1/(n1+n2) to a mass frac (m1/(m1+m2) given
  // corresponding compositions c1 and c2.
  double AtomToMassFrac(double atomfrac, Composition::Ptr c1,
                        Composition::Ptr c2) {
    cyclus::CompMap n1 = c1->atom();
    cyclus::CompMap n2 = c2->atom();
    cyclus::compmath::Normalize(&n1, atomfrac);
    cyclus::compmath::Normalize(&n2, 1 - atomfrac);

    cyclus::CompMap::iterator it;

    double mass1 = 0;
    for (it = n1.begin(); it != n1.end(); ++it) {
      mass1 += it->second * pyne::atomic_mass(it->first);
    }

    double mass2 = 0;
    for (it = n2.begin(); it != n2.end(); ++it) {
      mass2 += it->second * pyne::atomic_mass(it->first);
    }

    return mass1 / (mass1 + mass2);
  }

}  // namespace cyclass
