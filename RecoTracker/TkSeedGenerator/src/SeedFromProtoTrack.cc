#include "RecoTracker/TkSeedGenerator/interface/SeedFromProtoTrack.h"

#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"

#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"

#include "TrackingTools/MaterialEffects/interface/PropagatorWithMaterial.h"
#include "TrackingTools/Records/interface/TrackingComponentsRecord.h"

#include "MagneticField/Engine/interface/MagneticField.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"

#include "TrackingTools/TrajectoryState/interface/TrajectoryStateTransform.h"
#include "DataFormats/TrackReco/interface/Track.h"



SeedFromProtoTrack::SeedFromProtoTrack(const reco::Track & proto,  const edm::EventSetup& es)
  : theValid(true)
{
  edm::ESHandle<TrackerGeometry> tracker;
  es.get<TrackerDigiGeometryRecord>().get(tracker);

  edm::ESHandle<Propagator>  propagatorHandle;
  es.get<TrackingComponentsRecord>().get("PropagatorWithMaterial",propagatorHandle);
  const Propagator*  propagator = &(*propagatorHandle);

  edm::ESHandle<MagneticField> field;
  es.get<IdealMagneticFieldRecord>().get(field);

  const TrackingRecHit* hit = 0;
  for (unsigned int iHit = 0, nHits = proto.recHitsSize(); iHit < nHits; ++iHit) {
    TrackingRecHitRef refHit = proto.recHit(iHit);
    hit = &(*refHit);
    theHits.push_back( hit->clone() );
  }

  reco::TrackBase::Point  vtx = proto.referencePoint();
  reco::TrackBase::Vector mom = proto.momentum();
  GlobalTrajectoryParameters gtp( 
      GlobalPoint(vtx.x(),vtx.y(),vtx.z()),
      GlobalVector(mom.x(),mom.y(),mom.z()),
      proto.charge(),  &(*field) ); 

  CurvilinearTrajectoryError err = proto.covariance();

  FreeTrajectoryState fts( gtp, err);

  TrajectoryStateOnSurface outerState =
      propagator->propagate(fts, tracker->idToDet(hit->geographicalId())->surface());

  TrajectoryStateTransform transformer;
  thePTraj = boost::shared_ptr<PTrajectoryStateOnDet>(
      transformer.persistentState(outerState, hit->geographicalId().rawId()) );


}

TrajectorySeed SeedFromProtoTrack::trajectorySeed() const 
{
  return TrajectorySeed( trajectoryState(), hits(), direction());
}
