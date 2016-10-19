#include "fuel_fab.h"

#include <gtest/gtest.h>
#include <sstream>
#include "cyclus.h"

using pyne::nucname::id;
using cyclus::Composition;
using cyclus::CompMap;
using cyclus::Material;
using cyclus::QueryResult;
using cyclus::Cond;
using cyclus::toolkit::MatQuery;

namespace cyclass {
namespace fuelfabtests {

Composition::Ptr c_uox() {
  CompMap m;
  m[id("u235")] = 0.04;
  m[id("u238")] = 0.96;
  return Composition::CreateFromMass(m);
};

Composition::Ptr c_mox() {
  CompMap m;
  m[id("u235")] = .7;
  m[id("u238")] = 100;
  m[id("pu239")] = 3.3;
  return Composition::CreateFromMass(m);
};

Composition::Ptr c_natu() {
  CompMap m;
  m[id("u235")] =  .007;
  m[id("u238")] =  .993;
  return Composition::CreateFromMass(m);
};

Composition::Ptr c_pustream() {
  CompMap m;
  m[id("pu239")] = 100;
  m[id("pu240")] = 10;
  m[id("pu241")] = 1;
  m[id("pu242")] = 1;
  return Composition::CreateFromMass(m);
};

Composition::Ptr c_pustreamlow() {
  CompMap m;
  m[id("pu239")] = 80;
  m[id("pu240")] = 10;
  m[id("pu241")] = 1;
  m[id("pu242")] = 1;
  return Composition::CreateFromMass(m);
};

Composition::Ptr c_pustreambad() {
  CompMap m;
  m[id("pu239")] = 1;
  m[id("pu240")] = 10;
  m[id("pu241")] = 1;
  m[id("pu242")] = 1;
  return Composition::CreateFromMass(m);
};

Composition::Ptr c_water() {
  CompMap m;
  m[id("O16")] =  1;
  m[id("H1")] =  2;
  return Composition::CreateFromAtom(m);
};



// request (and receive) a specific recipe for fissile stream correctly.
TEST(FuelFabTests, FissRecipe) {
  std::string config = 
     "<fill_commods> <val>dummy</val> </fill_commods>"
     "<fill_recipe>natu</fill_recipe>"
     "<fill_size>1</fill_size>"
     ""
     "<fiss_commods> <val>stream1</val> <val>stream2</val> <val>stream3</val> </fiss_commods>"
     "<fiss_size>2.5</fiss_size>"
     "<fiss_recipe>spentuox</fiss_recipe>"
     ""
     "<outcommod>dummyout</outcommod>"
     "<spectrum>thermal</spectrum>"
     "<throughput>0</throughput>"
     ;

  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec(":cyclass:FuelFab"), config, simdur);
  sim.AddSource("stream1").Finalize();
  sim.AddRecipe("spentuox", c_pustream());
  sim.AddRecipe("natu", c_natu());
  int id = sim.Run();

  int resid = sim.db().Query("Transactions", NULL).GetVal<int>("ResourceId");
  CompMap got = sim.GetMaterial(resid)->comp()->mass();
  CompMap want = c_pustream()->mass();
  cyclus::compmath::Normalize(&got);
  cyclus::compmath::Normalize(&want);
  CompMap::iterator it;
  for (it = want.begin(); it != want.end(); ++it) {
    EXPECT_DOUBLE_EQ(it->second, got[it->first]) << "nuclide qty off: " << pyne::nucname::name(it->first);
  }
}

// multiple fissile streams can be correctly requested and used as
// fissile material inventory.
TEST(FuelFabTests, MultipleFissStreams) {
  std::string config = 
     "<fill_commods> <val>dummy</val> </fill_commods>"
     "<fill_recipe>natu</fill_recipe>"
     "<fill_size>1</fill_size>"
     ""
     "<fiss_commods> <val>stream1</val> <val>stream2</val> <val>stream3</val> </fiss_commods>"
     "<fiss_size>2.5</fiss_size>"
     ""
     "<outcommod>dummyout</outcommod>"
     "<spectrum>thermal</spectrum>"
     "<throughput>0</throughput>"
     ;

  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec(":cyclass:FuelFab"), config, simdur);
  sim.AddSource("stream1").recipe("spentuox").capacity(1).Finalize();
  sim.AddSource("stream2").recipe("spentuox").capacity(1).Finalize();
  sim.AddSource("stream3").recipe("spentuox").capacity(1).Finalize();
  sim.AddRecipe("spentuox", c_pustream());
  sim.AddRecipe("natu", c_natu());
  int id = sim.Run();

  QueryResult qr = sim.db().Query("Transactions", NULL);
  EXPECT_EQ(3, qr.rows.size());

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("stream1")));
  qr = sim.db().Query("Transactions", &conds);
  EXPECT_EQ(1, qr.rows.size());

  conds[0] = Cond("Commodity", "==", std::string("stream2"));
  qr = sim.db().Query("Transactions", &conds);
  EXPECT_EQ(1, qr.rows.size());

  conds[0] = Cond("Commodity", "==", std::string("stream3"));
  qr = sim.db().Query("Transactions", &conds);
  EXPECT_EQ(1, qr.rows.size());
}

// fissile stream preferences can be specified.
TEST(FuelFabTests, FissStreamPrefs) {
  std::string config = 
     "<fill_commods> <val>dummy</val> </fill_commods>"
     "<fill_recipe>natu</fill_recipe>"
     "<fill_size>1</fill_size>"
     ""
     "<fiss_commods>      <val>stream1</val> <val>stream2</val> <val>stream3</val> </fiss_commods>"
     "<fiss_commod_prefs> <val>1.0</val>     <val>0.1</val>     <val>2.0</val> </fiss_commod_prefs>"
     "<fiss_size>1.5</fiss_size>"
     ""
     "<outcommod>dummyout</outcommod>"
     "<spectrum>thermal</spectrum>"
     "<throughput>0</throughput>"
     ;

  int simdur = 1;
  cyclus::MockSim sim(cyclus::AgentSpec(":cyclass:FuelFab"), config, simdur);
  sim.AddSource("stream1").recipe("spentuox").capacity(1).Finalize();
  sim.AddSource("stream2").recipe("spentuox").capacity(1).Finalize();
  sim.AddSource("stream3").recipe("spentuox").capacity(1).Finalize();
  sim.AddRecipe("spentuox", c_pustream());
  sim.AddRecipe("natu", c_natu());
  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("stream1")));
  QueryResult qr = sim.db().Query("Transactions", &conds);
  Material::Ptr m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));
  EXPECT_DOUBLE_EQ(0.5, m->quantity());

  conds[0] = Cond("Commodity", "==", std::string("stream2"));
  qr = sim.db().Query("Transactions", &conds);
  EXPECT_EQ(0, qr.rows.size());

  conds[0] = Cond("Commodity", "==", std::string("stream3"));
  qr = sim.db().Query("Transactions", &conds);
  m = sim.GetMaterial(qr.GetVal<int>("ResourceId"));
  EXPECT_DOUBLE_EQ(1.0, m->quantity());
}

// zero throughput must not result in a zero capacity constraint excception.
TEST(FuelFabTests, ZeroThroughput) {
  std::string config = 
     "<fill_commods> <val>natu</val> </fill_commods>"
     "<fill_recipe>natu</fill_recipe>"
     "<fill_size>3.9</fill_size>"
     ""
     "<fiss_commods> <val>spentuox</val> </fiss_commods>"
     "<fiss_recipe>spentuox</fiss_recipe>"
     "<fiss_size>3.5</fiss_size>"
     ""
     "<topup_commod>uox</topup_commod>"
     "<topup_recipe>uox</topup_recipe>"
     "<topup_size>3.3</topup_size>"
     ""
     "<outcommod>dummyout</outcommod>"
     "<spectrum>thermal</spectrum>"
     "<throughput>0</throughput>"
     ;

  int simdur = 10;
  cyclus::MockSim sim(cyclus::AgentSpec(":cyclass:FuelFab"), config, simdur);
  sim.AddSource("uox").capacity(1).Finalize();
  sim.AddSource("spentuox").capacity(1).Finalize();
  sim.AddSource("natu").capacity(1).Finalize();
  sim.AddRecipe("uox", c_uox());
  sim.AddRecipe("spentuox", c_pustream());
  sim.AddRecipe("natu", c_natu());
  EXPECT_NO_THROW(sim.Run());
}

// fill, fiss, and topup inventories are all requested for and
// filled as expected. Inventory size constraints are properly
// enforced after they are full.
TEST(FuelFabTests, FillAllInventories) {
  std::string config = 
     "<fill_commods> <val>natu</val> </fill_commods>"
     "<fill_recipe>natu</fill_recipe>"
     "<fill_size>3.9</fill_size>"
     ""
     "<fiss_commods> <val>spentuox</val> </fiss_commods>"
     "<fiss_recipe>spentuox</fiss_recipe>"
     "<fiss_size>3.5</fiss_size>"
     ""
     "<outcommod>dummyout</outcommod>"
     "<throughput>1</throughput>"
     ;

  int simdur = 10;
  cyclus::MockSim sim(cyclus::AgentSpec(":cyclass:FuelFab"), config, simdur);
  sim.AddSource("uox").capacity(1).Finalize();
  sim.AddSource("spentuox").capacity(1).Finalize();
  sim.AddSource("natu").capacity(1).Finalize();
  sim.AddRecipe("uox", c_uox());
  sim.AddRecipe("spentuox", c_pustream());
  sim.AddRecipe("natu", c_natu());
  int id = sim.Run();

  cyclus::SqlStatement::Ptr stmt = sim.db().db().Prepare(
      "SELECT SUM(r.Quantity) FROM Transactions AS t"
      " INNER JOIN Resources AS r ON r.ResourceId = t.ResourceId"
      " WHERE t.Commodity = ?;"
      );

  stmt->BindText(1, "natu");
  stmt->Step();
  EXPECT_DOUBLE_EQ(3.9, stmt->GetDouble(0));
  stmt->Reset();
  stmt->BindText(1, "spentuox");
  stmt->Step();
  EXPECT_DOUBLE_EQ(3.5, stmt->GetDouble(0));
}

// Meet a request requiring zero fill inventory when we have zero fill
// inventory quantity.
/*TEST(FuelFabTests, ProvideStraightFiss_WithZeroFill) {
  std::string config = 
     "<fill_commods> <val>nothing</val> </fill_commods>"
     "<fill_recipe>natu</fill_recipe>"
     "<fill_size>100</fill_size>"
     ""
     "<fiss_commods> <val>anything</val> </fiss_commods>"
     "<fiss_recipe>spentuox</fiss_recipe>"
     "<fiss_size>100</fiss_size>"
     ""
     "<outcommod>recyclefuel</outcommod>"
     "<throughput>100</throughput>"
     ;
  int simdur = 6;
  cyclus::MockSim sim(cyclus::AgentSpec(":cyclass:FuelFab"), config, simdur);
  sim.AddSource("anything").Finalize();
  sim.AddSink("recyclefuel").recipe("spentuox").capacity(100).Finalize();
  sim.AddRecipe("uox", c_uox());
  sim.AddRecipe("spentuox", c_pustream());
  sim.AddRecipe("natu", c_natu());
  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("recyclefuel")));
  QueryResult qr = sim.db().Query("Transactions", NULL);
  // 6 = 3 receives of inventory, 3 sells of recycled fuel
  EXPECT_EQ(6, qr.rows.size());
}
*/
/*TEST(FuelFabTests, ProvideStraightFill_ZeroFiss) {
  std::string config = 
     "<fill_commods> <val>anything</val> </fill_commods>"
     "<fill_recipe>natu</fill_recipe>"
     "<fill_size>100</fill_size>"
     ""
     "<fiss_commods> <val>nothing</val> </fiss_commods>"
     "<fiss_recipe>spentuox</fiss_recipe>"
     "<fiss_size>100</fiss_size>"
     ""
     "<outcommod>recyclefuel</outcommod>"
     "<spectrum>thermal</spectrum>"
     "<throughput>100</throughput>"
     ;
  int simdur = 6;
  cyclus::MockSim sim(cyclus::AgentSpec(":cyclass:FuelFab"), config, simdur);
  sim.AddSource("anything").Finalize();
  sim.AddSink("recyclefuel").recipe("natu").capacity(100).Finalize();
  sim.AddRecipe("uox", c_uox());
  sim.AddRecipe("spentuox", c_pustream());
  sim.AddRecipe("natu", c_natu());
  int id = sim.Run();

  std::vector<Cond> conds;
  conds.push_back(Cond("Commodity", "==", std::string("recyclefuel")));
  QueryResult qr = sim.db().Query("Transactions", NULL);
  // 6 = 3 receives of inventory, 3 sells of recycled fuel
  EXPECT_EQ(6, qr.rows.size());
}*/

// throughput is properly restricted when faced with many fuel
// requests and with ample material inventory.
TEST(FuelFabTests, ThroughputLimit) {
  std::string config = 
     "<fill_commods> <val>anything</val> </fill_commods>"
     "<fill_recipe>natu</fill_recipe>"
     "<fill_size>100</fill_size>"
     ""
     "<fiss_commods> <val>anything</val> </fiss_commods>"
     "<fiss_recipe>pustream</fiss_recipe>"
     "<fiss_size>100</fiss_size>"
     ""
     "<outcommod>recyclefuel</outcommod>"
     "<spectrum>thermal</spectrum>"
     "<throughput>3</throughput>"
     ;
  double throughput = 3;

  int simdur = 5;
  cyclus::MockSim sim(cyclus::AgentSpec(":cyclass:FuelFab"), config, simdur);
  sim.AddSource("anything").lifetime(1).Finalize();
  sim.AddSink("recyclefuel").recipe("uox").capacity(2*throughput).Finalize();
  sim.AddRecipe("uox", c_uox());
  sim.AddRecipe("pustream", c_pustream());
  sim.AddRecipe("natu", c_natu());
  int id = sim.Run();

  QueryResult qr = sim.db().Query("Transactions", NULL);
  EXPECT_LT(2, qr.rows.size());

  cyclus::SqlStatement::Ptr stmt = sim.db().db().Prepare(
      "SELECT SUM(r.Quantity) FROM Transactions AS t"
      " INNER JOIN Resources AS r ON r.ResourceId = t.ResourceId"
      " WHERE t.Commodity = 'recyclefuel';"
      );

  stmt->Step();
  EXPECT_DOUBLE_EQ(throughput * (simdur-1), stmt->GetDouble(0));

  stmt = sim.db().db().Prepare(
      "SELECT COUNT(*) FROM Transactions WHERE Commodity = 'recyclefuel';"
      );

  stmt->Step();
  EXPECT_DOUBLE_EQ(simdur-1, stmt->GetDouble(0));
}


// Before this test and a fix, the fuel fab (partially) assumed each entire material
// buffer had the same composition as the material on top of the buffer when
// calculating stream mixing ratios for material to supply.  This problem was
// compounded by the fact that material weights are computed on an atom basis
// and mixing is done on a mass basis - corresponding conversions resulted in
// the fab being matched for more than it could actually supply - due to
// thinking it had an inventory of higher quality material than was actually
// the case.  This test makes sure that doesn't happen again.
/*TEST(FuelFabTests, HomogenousBuffers) {
  std::string config = 
     "<fill_commods> <val>natu</val> </fill_commods>"
     "<fill_recipe>natu</fill_recipe>"
     "<fill_size>40</fill_size>"
     ""
     "<fiss_commods> <val>stream1</val> </fiss_commods>"
     "<fiss_size>4</fiss_size>"
     "<fiss_recipe>spentuox</fiss_recipe>"
     ""
     "<outcommod>out</outcommod>"
     "<spectrum>thermal</spectrum>"
     "<throughput>1e10</throughput>"
     ;

  CompMap m;
  m[id("u235")] = 7;
  m[id("u238")] = 86;
  // the zr90 is important to force the atom-mass basis conversion to push the
  // dre to overmatch in the direction we want.
  m[id("zr90")] = 7;
  Composition::Ptr c = Composition::CreateFromMass(m);

  int simdur = 5;
  cyclus::MockSim sim(cyclus::AgentSpec(":cyclass:FuelFab"), config, simdur);
  sim.AddSource("stream1").start(0).lifetime(1).capacity(.01).recipe("special").Finalize();
  sim.AddSource("stream1").start(1).lifetime(1).capacity(3.98).recipe("natu").Finalize();
  sim.AddSource("natu").lifetime(1).Finalize();
  sim.AddSink("out").start(2).capacity(4).lifetime(1).recipe("uox").Finalize();
  sim.AddSink("out").start(2).capacity(4).lifetime(1).recipe("uox").Finalize();
  sim.AddRecipe("uox", c_uox());
  sim.AddRecipe("spentuox", c_pustream());
  sim.AddRecipe("natu", c_natu());
  sim.AddRecipe("special", c);
  ASSERT_NO_THROW(sim.Run());
}*/

} // namespace fuelfabtests
} // namespace cyclass


