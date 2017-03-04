#include "topology_generator/TopologyGenerator.h"

namespace SceneModel {

TopologyGenerator::TopologyGenerator(const std::vector<std::string>& pAllObjectTypes, unsigned int pMaxNeighbourCount):
    mAllObjectTypes(pAllObjectTypes), mMaxNeighbourCount(pMaxNeighbourCount), mRemoveRelations(true), mSwapRelations(true)
{
    mConnectivityChecker = boost::shared_ptr<ConnectivityChecker>(new ConnectivityChecker(pAllObjectTypes.size()));
}

std::vector<boost::shared_ptr<Topology>> TopologyGenerator::generateNeighbours(boost::shared_ptr<Topology> pFrom)
{
    bool useDisconnected = false;

    std::vector<bool> bitvector = convertTopologyToBitvector(pFrom);
    std::vector<std::vector<bool>> neighbours = calculateNeighbours(bitvector);
    std::vector<boost::shared_ptr<Topology>> result;
    std::cout << std::endl;
    for (std::vector<bool> i: neighbours)
    {
        for (bool bit: i)
        {
            if (bit) std::cout << "1";
            else std::cout << "0";
        }
        if (useDisconnected || mConnectivityChecker->isConnected(i))
        {
            std::cout << "c";
            result.push_back(convertBitvectorToTopology(i));
        }
        std::cout << " ";
    }
    std::cout << std::endl;
    return result;
}

std::vector<boost::shared_ptr<Topology>> TopologyGenerator::generateStarTopologies()
{
    std::vector<boost::shared_ptr<Topology>> result;
    /*for (std::string typeA: mAllObjectTypes)
    {
        std::vector<boost::shared_ptr<Relation>> relations;
        for (std::string typeB: mAllObjectTypes)
            if (typeA != typeB)
            {
                boost::shared_ptr<Relation> newRelation(new Relation(typeA, typeB)); // Generate all relations connecting the object of this type to all the others
                relations.push_back(newRelation);
            }
        boost::shared_ptr<Topology> newTopology(new Topology());
        newTopology->mRelations = relations;    // TODO: evaluation
        result.push_back(newTopology);
    }*/
    // from lib_ism:
    std::vector<std::vector<bool>> starBitVectors;
    unsigned int numObjects = mAllObjectTypes.size();
    unsigned int numAllRelations = (numObjects - 1) * numObjects / 2;

    for (unsigned int i = 0; i < numObjects; ++i)
    {
        std::vector<bool> star(numAllRelations, 0);
        unsigned int pos = 0;
        for (unsigned int j = 0; j < i; ++j) {
            star[pos + i - (j + 1)] = 1;
            pos += numObjects - (j + 1);
        }
        for (unsigned int k = pos; k < pos + numObjects - 1 - i; ++k) {
            star[k] = 1;
        }
        starBitVectors.push_back(star);
    }

    for (std::vector<bool> starBitVector: starBitVectors)
        result.push_back(convertBitvectorToTopology(starBitVector));
    return result;
}


boost::shared_ptr<Topology> TopologyGenerator::generateFullyMeshedTopology()
{
    unsigned int numObjects = mAllObjectTypes.size();
    /*for (std::string typeA: mAllObjectTypes)
        for (std::string typeB: mAllObjectTypes)
            if (typeA != typeB)
                {
                    boost::shared_ptr<Relation> newRelation(new Relation(typeA, typeB));
                    relations.push_back(newRelation);
                }
    boost::shared_ptr<Topology> result(new Topology());
    result->mRelations = relations;    // TODO: evaluation*/
    std::vector<bool> bitvector((numObjects - 1) * numObjects / 2, true);   // contains all possible relations.
    boost::shared_ptr<Topology> result = convertBitvectorToTopology(bitvector);
    return result;
}

boost::shared_ptr<Topology> TopologyGenerator::generateRandomTopology()
{
    unsigned int numAllObjects = mAllObjectTypes.size();
    unsigned int numAllRelations =  (numAllObjects - 1) * numAllObjects / 2;
    // from lib_ism:
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<unsigned int> dist(0, 1);

    std::vector<bool> relations(numAllRelations, 0);
    do
    {
        for (unsigned int i = 0; i < numAllRelations; i++) relations[i] = dist(eng);
    }
    while (!mConnectivityChecker->isConnected(relations));
    boost::shared_ptr<Topology> result = convertBitvectorToTopology(relations);
    return result;
}

std::vector<boost::shared_ptr<Topology>> TopologyGenerator::generateAllConnectedTopologies()
{
    std::vector<boost::shared_ptr<Topology>> result;
    boost::shared_ptr<SceneModel::Topology> fullyMeshed = generateFullyMeshedTopology();
    result.push_back(fullyMeshed);
    std::cout << "Trying to reach all possible topologies from the fully meshed one." << std::endl;
    std::vector<boost::shared_ptr<SceneModel::Topology>> neighboursToVisit;
    neighboursToVisit.push_back(fullyMeshed);
    std::vector<std::string> seenNeighbourIDs;
    seenNeighbourIDs.push_back(fullyMeshed->mIdentifier);
    unsigned int visitedCounter = 0;
    while(!neighboursToVisit.empty())
    {
        boost::shared_ptr<SceneModel::Topology> from = neighboursToVisit[0];
        neighboursToVisit.erase(neighboursToVisit.begin());

        std::cout << "Generating neighbours of topology " << from->mIdentifier << ":";
        std::vector<boost::shared_ptr<SceneModel::Topology>> newNeighbours = generateNeighbours(from);
        unsigned int uniqueCounter = 0;
        for (boost::shared_ptr<SceneModel::Topology> newNeighbour: newNeighbours)
        {
            std::cout << " " << newNeighbour->mIdentifier;
            if (std::find(seenNeighbourIDs.begin(), seenNeighbourIDs.end(), newNeighbour->mIdentifier) == seenNeighbourIDs.end())   // not yet visited
            {
                std::cout << "*";   // Mark unvisited.
                neighboursToVisit.push_back(newNeighbour);
                seenNeighbourIDs.push_back(newNeighbour->mIdentifier);
                result.push_back(newNeighbour);
                uniqueCounter++;
            }
        }
        std::cout << std::endl << newNeighbours.size() << " neighbours found. " << uniqueCounter << " not yet visited." << std::endl;
        std::cout << "===================================================" << std::endl;
        visitedCounter++;
    }
    std::cout << "Done." << std::endl;
    std::cout << "Visited " << visitedCounter << " topologies in total." << std::endl;
    std::cout << "===================================================" << std::endl;

    return result;
}


// currently unused
std::vector<std::vector<bool>> TopologyGenerator::selectRandomNeighbours(std::vector<std::vector<bool>>& pNeighbours)
{
    if (pNeighbours.size() >= mMaxNeighbourCount) return pNeighbours;   // if amount of neighbours is already smaller than maximum amount: return neighbours
    //from lib_ism: Sort from by #relations.
    struct compare {
        bool operator() (const std::vector<bool> & first, const std::vector<bool> & second) {
            return first.size() > second.size();
        }
    };

    std::sort(pNeighbours.begin(), pNeighbours.end(), compare());

    std::random_device rd;
    std::mt19937 eng(rd());
    std::normal_distribution<double> dist(0, pNeighbours.size() / 2);

    std::vector<std::vector<bool>> selectedNeighbours;

    for (unsigned int i = 0; i < mMaxNeighbourCount; i++)
    {
        unsigned int randIndex;
        do {
            randIndex = std::abs(dist(eng));
        } while (randIndex >= pNeighbours.size());

    selectedNeighbours.push_back(pNeighbours[randIndex]);
    pNeighbours.erase(pNeighbours.begin() + randIndex);

    }

    return selectedNeighbours;
}

boost::shared_ptr<Topology> TopologyGenerator::convertBitvectorToTopology(const std::vector<bool> & pBitvector)
{
    unsigned int numObjectTypes = mAllObjectTypes.size();
    std::vector<boost::shared_ptr<Relation>> relations;
    unsigned int bitvectorindex = 0;
    for (unsigned int i = 0; i < numObjectTypes - 1; i++)
    {
        //std::cout << "i = " << i << ": ";
        std::string typeA = mAllObjectTypes[i];
        //std::cout << "typeA = " << typeA << ". " << std::endl;
        for (unsigned int j = i + 1; j < numObjectTypes; j++)
        {
            //std::cout << "j = " << j << ": ";
            std::string typeB = mAllObjectTypes[j];
            //std::cout << "typeB = " << typeB << ". ";
            if (pBitvector[bitvectorindex])
            {
                boost::shared_ptr<Relation> newRelation(new Relation(typeA, typeB));
                relations.push_back(newRelation);
            }
            //std::cout << "bitvectorindex = " << bitvectorindex << "." << std::endl;
            bitvectorindex++;
        }
    }
    boost::shared_ptr<Topology> result(new Topology());
    result->mRelations = relations;
    // set result's identifier:
    std::string identifier = "";
    for (bool bit: pBitvector)
    {
        if (bit) identifier += "1";
        else identifier += "0";
    }
    result->mIdentifier = identifier;
    return result;
}

std::vector<bool> TopologyGenerator::convertTopologyToBitvector(boost::shared_ptr<Topology> pTopology)
{
    //std::cout << std::endl;
    unsigned int numObjectTypes = mAllObjectTypes.size();
    unsigned int numAllRelations = (numObjectTypes - 1) * numObjectTypes / 2;
    std::vector<bool> result = std::vector<bool>(numAllRelations, false);
    std::map<std::string, unsigned int> indicesByTypes;
    for (unsigned int i = 0; i < numObjectTypes; i++)
        indicesByTypes[mAllObjectTypes[i]] = i;

    std::vector<boost::shared_ptr<Relation>> relations = pTopology->mRelations;
    for (boost::shared_ptr<Relation> relation: relations)
    {
        std::string typeA = relation->getObjectTypeA();
        std::string typeB = relation->getObjectTypeB();
        unsigned int indexA = indicesByTypes[typeA];
        unsigned int indexB = indicesByTypes[typeB];
        unsigned int indexMin = std::min(indexA, indexB);
        unsigned int indexMax = std::max(indexA, indexB);
        unsigned int bitvectorindex = 0;
        //unsigned int relevantbitvectorindex = 0;
        /*for (unsigned int i = 0; i < numObjectTypes - 1; i++)
            for (unsigned int j = i + 1; j < numObjectTypes; j++)
            {
                if (indexMin == i && indexMax == j)
                {
                    result[bitvectorindex] = true;
                    relevantbitvectorindex = bitvectorindex;
                    std::cout << "current bitvector: ";
                    for (bool bit: result)
                    {
                        if (bit) std::cout << "1";
                        else std::cout << "0";
                    }
                    std::cout << std::endl;
                    break;
                }
                bitvectorindex++;
            }*/
        unsigned int testbitvectorindex = indexMin * numObjectTypes - indexMin * (indexMin + 1) / 2 + (indexMax - indexMin) - 1;
        /*std::cout << "indexMin * numObjectTypes - indexMin * (indexMin + 1) / 2 + (indexMax - indexMin) - 1 = " << indexMin << " * " << numObjectTypes << " - " <<
                     indexMin << " * (" << indexMin  << " + 1) / 2 + (" << indexMax << " - " << indexMin << ") - 1 = " <<
                     indexMin * numObjectTypes - indexMin * (indexMin + 1) / 2 + (indexMax - indexMin) - 1 << std::endl;
        //std::cout << "bitvectorindex = " << relevantbitvectorindex << ". testbitvectorindex = " << testbitvectorindex << std::endl;*/
        bitvectorindex = testbitvectorindex;
        result[bitvectorindex] = true;
    }
    //std::cout << "---" << std::endl;
    return result;
}

std::vector<std::vector<bool>> TopologyGenerator::calculateNeighbours(std::vector<bool> pFrom)
{
    // from TopologyGeneratorPaper in lib_ism:
    std::vector<std::vector<bool>> neighbours;
    std::vector<unsigned int> ones;
    std::vector<unsigned int> zeros;

    for (unsigned int i = 0; i < pFrom.size(); ++i)
    {
        if (pFrom[i])
        {
            ones.push_back(i);
        } else {
            zeros.push_back(i);
        }
    }

    std::vector<bool> neighbour;
    if (mRemoveRelations)
    {
        //Remove one relation
        for (unsigned int i = 0; i < ones.size(); ++i)
        {
            neighbour = pFrom;
            neighbour[ones[i]] = 0;
            neighbours.push_back(neighbour);
        }
    }

    for (unsigned int i = 0; i < zeros.size(); ++i)
    {
        //Add one relation
        neighbour = pFrom;
        neighbour[zeros[i]] = 1;
        neighbours.push_back(neighbour);

        if (mSwapRelations)
        {
            //Swap one relation
            for (unsigned int j = 0; j < ones.size(); ++j)
            {
                neighbour = pFrom;
                neighbour[zeros[i]] = 1;
                neighbour[ones[j]] = 0;
                neighbours.push_back(neighbour);
            }
        }
    }

    return neighbours;
}

}
