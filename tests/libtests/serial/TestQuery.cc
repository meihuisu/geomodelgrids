/**
 * C++ unit testing of geomodelgrids::serial::Query.
 */

#include <portinfo>

#include "ModelPoints.hh"

#include "geomodelgrids/serial/Query.hh" // USES Query
#include "geomodelgrids/utils/constants.hh" // USES NODATA_VALUE

#include <cppunit/extensions/HelperMacros.h>

#include <cmath>

namespace geomodelgrids {
    namespace serial {
        class TestQuery;
    } // serial
} // geomodelgrids

class geomodelgrids::serial::TestQuery : public CppUnit::TestFixture {
    // CPPUNIT TEST SUITE /////////////////////////////////////////////////
    CPPUNIT_TEST_SUITE(TestQuery);

    CPPUNIT_TEST(testConstructor);
    CPPUNIT_TEST(testAccessors);
    CPPUNIT_TEST(testInitialize);
    CPPUNIT_TEST(testQueryElevation);
    CPPUNIT_TEST(testQueryFlat);
    CPPUNIT_TEST(testQueryTopo);
    CPPUNIT_TEST(testQuerySquash);

    CPPUNIT_TEST_SUITE_END();

    // PUBLIC METHODS ///////////////////////////////////////////////////////
public:

    /// Test constructor.
    void testConstructor(void);

    /// Test getters.
    void testAccessors(void);

    /// Test initialize() and finalize().
    void testInitialize(void);

    /// Test queryElevation().
    void testQueryElevation(void);

    /// Test query() for model with flat ground surface.
    void testQueryFlat(void);

    /// Test query() for model with topography.
    void testQueryTopo(void);

    /// Test query() for model with topography using squashing.
    void testQuerySquash(void);

}; // class TestQuery
CPPUNIT_TEST_SUITE_REGISTRATION(geomodelgrids::serial::TestQuery);

// ----------------------------------------------------------------------
// Test constructor.
void
geomodelgrids::serial::TestQuery::testConstructor(void) {
    Query query;

    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("Mismatch in default minimum squashing elevation.", 0.0, query._squashMinElev,
                                         1.0e-6);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Mismatch in default squahing flag.", false, query._squash);
} // testConstructor


// ----------------------------------------------------------------------
// Test setters.
void
geomodelgrids::serial::TestQuery::testAccessors(void) {
    Query query;

    const double minElev(-2.0e+3);
    query.setSquashMinElev(minElev);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Mismatch in squashing flag.", true, query._squash);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("Mismatch in minimum squashing elevation.", minElev, query._squashMinElev,
                                         1.0e-6);

    query.setSquashing(false);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Mismatch in squashing flag.", false, query._squash);
} // testAccessors


// ---------------------------------------------------------------------------------------------------------------------
// Test initialize().
void
geomodelgrids::serial::TestQuery::testInitialize(void) {
    const size_t numModels = 2;
    const char* const filenamesArray[numModels] = {
        "../../data/one-block-topo.h5",
        "../../data/three-blocks-topo.h5",
    };
    std::vector<std::string> filenames(filenamesArray, filenamesArray+numModels);

    const size_t numValues = 2;
    const char* const valueNamesArray[numValues] = { "two", "one" };
    std::vector<std::string> valueNames(valueNamesArray, valueNamesArray+numValues);
    const size_t valueIndexE[numValues] = { 1, 0 };

    const char* const crs = "EPSG:4326";

    Query query;
    query.initialize(filenames, valueNames, crs);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Mismatch in number of models.", numModels, query._models.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Mismatch in size of model values map.", numModels, query._valuesIndex.size());
    for (size_t iModel = 0; iModel < numModels; ++iModel) {
        CPPUNIT_ASSERT(query._models[iModel]);

        CPPUNIT_ASSERT_EQUAL_MESSAGE("Mismatch in number of values.", numValues, query._valuesIndex[iModel].size());
        for (size_t iValue = 0; iValue < numValues; ++iValue) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Mismatch in value index.", valueIndexE[iValue], query._valuesIndex[iModel][iValue]);
        } // for

    } // for

    CPPUNIT_ASSERT_NO_THROW(query.initialize(filenames, valueNames, crs));
    query.finalize();

    // Bad value
    const char* const valueNamesArrayBad[numValues] = { "two", "blah" };
    std::vector<std::string> valueNamesBad(valueNamesArrayBad, valueNamesArrayBad+numValues);
    CPPUNIT_ASSERT_THROW(query.initialize(filenames, valueNamesBad, crs), std::invalid_argument);
} // testInitialize


// ---------------------------------------------------------------------------------------------------------------------
// Test queryElevation().
void
geomodelgrids::serial::TestQuery::testQueryElevation(void) {
    const size_t numModels = 2;
    const char* const filenamesArray[numModels] = {
        "../../data/one-block-flat.h5",
        "../../data/three-blocks-topo.h5",
    };
    std::vector<std::string> filenames(filenamesArray, filenamesArray+numModels);

    geomodelgrids::testdata::OneBlockFlatPoints pointsOne;
    const std::string& crs = pointsOne.getCRSLonLatElev();
    const size_t spaceDim = 3;

    Query query;
    std::vector<std::string> valueNames;
    query.initialize(filenames, valueNames, crs);

    { // One Block Flat
        const size_t numPoints = pointsOne.getNumPoints();
        const double* pointsLLE = pointsOne.getLatLonElev();

        for (size_t iPt = 0; iPt < numPoints; ++iPt) {
            const double elevationE = 0.0;
            const double elevation = query.queryElevation(pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1]);

            std::ostringstream msg;
            msg << "Mismatch for point in one-block-flat ("
                << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1] << ").";
            const double tolerance = 1.0e-6;
            const double valueTolerance = std::max(tolerance, tolerance*fabs(elevationE));
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), elevationE, elevation, valueTolerance);
        } // for
    } // One Block Flat

    { // Three Blocks Topo
        geomodelgrids::testdata::ThreeBlocksTopoPoints pointsThree;
        const size_t numPoints = pointsThree.getNumPoints();
        const double* pointsLLE = pointsThree.getLatLonElev();
        const double* pointsXYZ = pointsThree.getXYZ();

        for (size_t iPt = 0; iPt < numPoints; ++iPt) {
            const double elevationE = pointsThree.computeElevation(pointsXYZ[iPt*spaceDim+0], pointsXYZ[iPt*spaceDim+1]);
            const double elevation = query.queryElevation(pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1]);

            std::ostringstream msg;
            msg << "Mismatch for point in three-blocks-topo ("
                << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1] << ").";
            const double tolerance = 1.0e-6;
            const double valueTolerance = std::max(tolerance, tolerance*fabs(elevationE));
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), elevationE, elevation, valueTolerance);
        } // for
    } // Three Blocks Topo

    { // Outside domains
        geomodelgrids::testdata::OutsideDomainPoints pointsOutside;
        const size_t numPoints = pointsOutside.getNumPoints();
        const double* pointsLLE = pointsOutside.getLatLonElev();

        // Skip first 2 points with z causing point to be outside domain.
        for (size_t iPt = 2; iPt < numPoints; ++iPt) {
            const double elevationE = NODATA_VALUE;
            const double elevation = query.queryElevation(pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1]);

            std::ostringstream msg;
            msg << "Mismatch for point outside domains ("
                << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1] << ").";
            const double tolerance = 1.0e-6;
            const double valueTolerance = std::max(tolerance, tolerance*fabs(elevationE));
            CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), elevationE, elevation, valueTolerance);
        } // for
    } // Outside domains
} // testQueryElevation


// ---------------------------------------------------------------------------------------------------------------------
// Test query() for model with flat ground surface.
void
geomodelgrids::serial::TestQuery::testQueryFlat(void) {
    const size_t numModels = 2;
    const char* const filenamesArray[numModels] = {
        "../../data/one-block-flat.h5",
        "../../data/three-blocks-flat.h5",
    };
    std::vector<std::string> filenames(filenamesArray, filenamesArray+numModels);

    const size_t numValues = 2;
    const char* const valueNamesArray[numValues] = { "two", "one" };
    std::vector<std::string> valueNames(valueNamesArray, valueNamesArray+numValues);

    geomodelgrids::testdata::OneBlockFlatPoints pointsOne;
    const std::string& crs = pointsOne.getCRSLonLatElev();
    const size_t spaceDim = 3;

    Query query;
    query.initialize(filenames, valueNames, crs);

    { // One Block Flat
        const size_t numPoints = pointsOne.getNumPoints();
        const double* pointsLLE = pointsOne.getLatLonElev();
        const double* pointsXYZ = pointsOne.getXYZ();

        for (size_t iPt = 0; iPt < numPoints; ++iPt) {
            double values[numValues];
            const int err = query.query(values, pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1], pointsLLE[iPt*spaceDim+2]);
            CPPUNIT_ASSERT_MESSAGE("Mismatch in err value in one-block-flat", !err);

            const double x = pointsXYZ[iPt*spaceDim+0];
            const double y = pointsXYZ[iPt*spaceDim+1];
            const double z = pointsXYZ[iPt*spaceDim+2];
            double valuesE[numValues];
            valuesE[0] = pointsOne.computeValueTwo(x, y, z);
            valuesE[1] = pointsOne.computeValueOne(x, y, z);

            for (size_t iValue = 0; iValue < numValues; ++iValue) {
                std::ostringstream msg;
                msg << "Mismatch at point (" << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1]
                    << ", " << pointsLLE[iPt*spaceDim+2] << ") for value '" << valueNames[iValue] << "' in one-block-flat.";
                const double tolerance = 1.0e-6;
                const double toleranceV = std::max(tolerance, tolerance*fabs(valuesE[iValue]));
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), valuesE[iValue], values[iValue],
                                                     toleranceV);
            } // for
        } // for
    } // One Block Flat

    { // Three Block Flat
        geomodelgrids::testdata::ThreeBlocksFlatPoints pointsThree;
        const size_t numPoints = pointsThree.getNumPoints();
        const double* pointsLLE = pointsThree.getLatLonElev();
        const double* pointsXYZ = pointsThree.getXYZ();

        for (size_t iPt = 0; iPt < numPoints; ++iPt) {
            double values[numValues];
            const int err = query.query(values, pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1], pointsLLE[iPt*spaceDim+2]);
            CPPUNIT_ASSERT_MESSAGE("Mismatch in err value in three-blocks-flat", !err);

            const double x = pointsXYZ[iPt*spaceDim+0];
            const double y = pointsXYZ[iPt*spaceDim+1];
            const double z = pointsXYZ[iPt*spaceDim+2];
            double valuesE[numValues];
            valuesE[0] = pointsThree.computeValueTwo(x, y, z);
            valuesE[1] = pointsThree.computeValueOne(x, y, z);

            for (size_t iValue = 0; iValue < numValues; ++iValue) {
                std::ostringstream msg;
                msg << "Mismatch at point (" << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1]
                    << ", " << pointsLLE[iPt*spaceDim+2] << ") for value '" << valueNames[iValue]
                    << "' in three-blocks-flat.";
                const double tolerance = 1.0e-6;
                const double toleranceV = std::max(tolerance, tolerance*fabs(valuesE[iValue]));
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), valuesE[iValue], values[iValue],
                                                     toleranceV);
            } // for
        } // for
    } // Three Block Flat

    { // Outside domain
        geomodelgrids::testdata::OutsideDomainPoints pointsOutisde;
        const size_t numPoints = pointsOutisde.getNumPoints();
        const double* pointsLLE = pointsOutisde.getLatLonElev();

        for (size_t iPt = 0; iPt < numPoints; ++iPt) {
            double values[numValues];
            const int err = query.query(values, pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1], pointsLLE[iPt*spaceDim+2]);
            CPPUNIT_ASSERT_MESSAGE("Mismatch in err value in outside-domain", err);

            for (size_t iValue = 0; iValue < numValues; ++iValue) {
                std::ostringstream msg;
                msg << "Mismatch at point (" << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1]
                    << ", " << pointsLLE[iPt*spaceDim+2] << ") for value '" << valueNames[iValue] << "' in outside-domain.";
                const double tolerance = 1.0e-6;
                const double toleranceV = std::max(tolerance, tolerance*fabs(NODATA_VALUE));
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), NODATA_VALUE, values[iValue],
                                                     toleranceV);
            } // for
        } // for
    } // Outside domain

} // testQueryFlat


// ---------------------------------------------------------------------------------------------------------------------
// Test query() for model with topography.
void
geomodelgrids::serial::TestQuery::testQueryTopo(void) {
    const size_t numModels = 2;
    const char* const filenamesArray[numModels] = {
        "../../data/one-block-topo.h5",
        "../../data/three-blocks-topo.h5",
    };
    std::vector<std::string> filenames(filenamesArray, filenamesArray+numModels);

    const size_t numValues = 2;
    const char* const valueNamesArray[numValues] = { "two", "one" };
    std::vector<std::string> valueNames(valueNamesArray, valueNamesArray+numValues);

    geomodelgrids::testdata::OneBlockTopoPoints pointsOne;
    const std::string& crs = pointsOne.getCRSLonLatElev();
    const size_t spaceDim = 3;

    Query query;
    query.initialize(filenames, valueNames, crs);

    { // One Block Topo
        const size_t numPoints = pointsOne.getNumPoints();
        const double* pointsLLE = pointsOne.getLatLonElev();
        const double* pointsXYZ = pointsOne.getXYZ();

        for (size_t iPt = 0; iPt < numPoints; ++iPt) {
            double values[numValues];
            const int err = query.query(values, pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1], pointsLLE[iPt*spaceDim+2]);
            CPPUNIT_ASSERT_MESSAGE("Mismatch in err value in one-block-topo.", !err);

            const double x = pointsXYZ[iPt*spaceDim+0];
            const double y = pointsXYZ[iPt*spaceDim+1];
            const double z = pointsXYZ[iPt*spaceDim+2];
            double valuesE[numValues];
            valuesE[0] = pointsOne.computeValueTwo(x, y, z);
            valuesE[1] = pointsOne.computeValueOne(x, y, z);

            for (size_t iValue = 0; iValue < numValues; ++iValue) {
                std::ostringstream msg;
                msg << "Mismatch at point (" << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1]
                    << ", " << pointsLLE[iPt*spaceDim+2] << ") for value '" << valueNames[iValue] << "' in one-block-topo.";
                const double tolerance = 1.0e-6;
                const double toleranceV = std::max(tolerance, tolerance*fabs(valuesE[iValue]));
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), valuesE[iValue], values[iValue],
                                                     toleranceV);
            } // for
        } // for
    } // One Block Topo

    { // Three Block Topo
        geomodelgrids::testdata::ThreeBlocksTopoPoints pointsThree;
        const size_t numPoints = pointsThree.getNumPoints();
        const double* pointsLLE = pointsThree.getLatLonElev();
        const double* pointsXYZ = pointsThree.getXYZ();

        for (size_t iPt = 0; iPt < numPoints; ++iPt) {
            double values[numValues];
            const int err = query.query(values, pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1], pointsLLE[iPt*spaceDim+2]);
            CPPUNIT_ASSERT_MESSAGE("Mismatch in err value in three-blocks-topo.", !err);

            const double x = pointsXYZ[iPt*spaceDim+0];
            const double y = pointsXYZ[iPt*spaceDim+1];
            const double z = pointsXYZ[iPt*spaceDim+2];
            double valuesE[numValues];
            valuesE[0] = pointsThree.computeValueTwo(x, y, z);
            valuesE[1] = pointsThree.computeValueOne(x, y, z);

            for (size_t iValue = 0; iValue < numValues; ++iValue) {
                std::ostringstream msg;
                msg << "Mismatch at point (" << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1]
                    << ", " << pointsLLE[iPt*spaceDim+2] << ") for value '" << valueNames[iValue]
                    << "' in three-blocks-topo.";
                const double tolerance = 1.0e-6;
                const double toleranceV = std::max(tolerance, tolerance*fabs(valuesE[iValue]));
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), valuesE[iValue], values[iValue],
                                                     toleranceV);
            } // for
        } // for
    } // Three Block Topo

    { // Outside domain
        geomodelgrids::testdata::OutsideDomainPoints pointsOutisde;
        const size_t numPoints = pointsOutisde.getNumPoints();
        const double* pointsLLE = pointsOutisde.getLatLonElev();

        for (size_t iPt = 0; iPt < numPoints; ++iPt) {
            double values[numValues];
            const int err = query.query(values, pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1], pointsLLE[iPt*spaceDim+2]);
            CPPUNIT_ASSERT_MESSAGE("Mismatch in err value in outside-domain.", err);

            for (size_t iValue = 0; iValue < numValues; ++iValue) {
                std::ostringstream msg;
                msg << "Mismatch at point (" << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1]
                    << ", " << pointsLLE[iPt*spaceDim+2] << ") for value '" << valueNames[iValue] << "' in outside-domain.";
                const double tolerance = 1.0e-6;
                const double toleranceV = std::max(tolerance, tolerance*fabs(NODATA_VALUE));
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), NODATA_VALUE, values[iValue],
                                                     toleranceV);
            } // for
        } // for
    } // Outside domain
} // testQueryTopo


// ---------------------------------------------------------------------------------------------------------------------
// Test query() for model with topography using squashing.
void
geomodelgrids::serial::TestQuery::testQuerySquash(void) {
    const double squashMinElev = -4.999e+3;

    const size_t numModels = 2;
    const char* const filenamesArray[numModels] = {
        "../../data/one-block-topo.h5",
        "../../data/three-blocks-topo.h5",
    };
    std::vector<std::string> filenames(filenamesArray, filenamesArray+numModels);

    const size_t numValues = 2;
    const char* const valueNamesArray[numValues] = { "two", "one" };
    std::vector<std::string> valueNames(valueNamesArray, valueNamesArray+numValues);

    geomodelgrids::testdata::OneBlockSquashPoints pointsOne(squashMinElev);
    const std::string& crs = pointsOne.getCRSLonLatElev();
    const size_t spaceDim = 3;

    Query query;
    query.setSquashMinElev(squashMinElev);
    query.initialize(filenames, valueNames, crs);

    { // One Block Squash
        const size_t numPoints = pointsOne.getNumPoints();
        const double* pointsLLE = pointsOne.getLatLonElev();
        const double* pointsXYZ = pointsOne.getXYZ();

        for (size_t iPt = 0; iPt < numPoints; ++iPt) {
            double values[numValues];
            const int err = query.query(values, pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1], pointsLLE[iPt*spaceDim+2]);
            CPPUNIT_ASSERT_MESSAGE("Mismatch in err value in one-block-topo.", !err);

            const double x = pointsXYZ[iPt*spaceDim+0];
            const double y = pointsXYZ[iPt*spaceDim+1];
            const double z = pointsXYZ[iPt*spaceDim+2];

            double valuesE[numValues];
            valuesE[0] = pointsOne.computeValueTwo(x, y, z);
            valuesE[1] = pointsOne.computeValueOne(x, y, z);

            for (size_t iValue = 0; iValue < numValues; ++iValue) {
                std::ostringstream msg;
                msg << "Mismatch at point (" << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1]
                    << ", " << pointsLLE[iPt*spaceDim+2] << ") for value '" << valueNames[iValue] << "' in one-block-topo.";
                const double tolerance = 1.0e-6;
                const double toleranceV = std::max(tolerance, tolerance*fabs(valuesE[iValue]));
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), valuesE[iValue], values[iValue],
                                                     toleranceV);
            } // for
        } // for
    } // One Block Squash

    { // Three Block Squash
        geomodelgrids::testdata::ThreeBlocksSquashPoints pointsThree(squashMinElev);
        const size_t numPoints = pointsThree.getNumPoints();
        const double* pointsLLE = pointsThree.getLatLonElev();
        const double* pointsXYZ = pointsThree.getXYZ();

        for (size_t iPt = 0; iPt < numPoints; ++iPt) {
            double values[numValues];
            const int err = query.query(values, pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1], pointsLLE[iPt*spaceDim+2]);
            CPPUNIT_ASSERT_MESSAGE("Mismatch in err value in three-blocks-topo.", !err);

            const double x = pointsXYZ[iPt*spaceDim+0];
            const double y = pointsXYZ[iPt*spaceDim+1];
            const double z = pointsXYZ[iPt*spaceDim+2];

            double valuesE[numValues];
            valuesE[0] = pointsThree.computeValueTwo(x, y, z);
            valuesE[1] = pointsThree.computeValueOne(x, y, z);

            for (size_t iValue = 0; iValue < numValues; ++iValue) {
                std::ostringstream msg;
                msg << "Mismatch at point (" << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1]
                    << ", " << pointsLLE[iPt*spaceDim+2] << ") for value '" << valueNames[iValue]
                    << "' in three-blocks-topo.";
                const double tolerance = 1.0e-6;
                const double toleranceV = std::max(tolerance, tolerance*fabs(valuesE[iValue]));
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), valuesE[iValue], values[iValue],
                                                     toleranceV);
            } // for
        } // for
    } // Three Block Squash

    { // Outside domain
        geomodelgrids::testdata::OutsideDomainPoints pointsOutisde;
        const size_t numPoints = pointsOutisde.getNumPoints();
        const double* pointsLLE = pointsOutisde.getLatLonElev();

        for (size_t iPt = 0; iPt < numPoints; ++iPt) {
            double values[numValues];
            const int err = query.query(values, pointsLLE[iPt*spaceDim+0], pointsLLE[iPt*spaceDim+1], pointsLLE[iPt*spaceDim+2]);
            CPPUNIT_ASSERT_MESSAGE("Mismatch in err value in outside-domain.", err);

            for (size_t iValue = 0; iValue < numValues; ++iValue) {
                std::ostringstream msg;
                msg << "Mismatch at point (" << pointsLLE[iPt*spaceDim+0] << ", " << pointsLLE[iPt*spaceDim+1]
                    << ", " << pointsLLE[iPt*spaceDim+2] << ") for value '" << valueNames[iValue] << "' in outside-domain.";
                const double tolerance = 1.0e-6;
                const double toleranceV = std::max(tolerance, tolerance*fabs(NODATA_VALUE));
                CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str().c_str(), NODATA_VALUE, values[iValue],
                                                     toleranceV);
            } // for
        } // for
    } // Outside domain
} // TestQuerySquash


// End of file