// PCL lib Functions for processing point clouds 

#include "processPointClouds.h"

//constructor:
template<typename PointT>
ProcessPointClouds<PointT>::ProcessPointClouds() {}


//de-constructor:
template<typename PointT>
ProcessPointClouds<PointT>::~ProcessPointClouds() {}


template<typename PointT>
void ProcessPointClouds<PointT>::numPoints(typename pcl::PointCloud<PointT>::Ptr cloud)
{
    std::cout << cloud->points.size() << std::endl;
}


template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr ProcessPointClouds<PointT>::FilterCloud(typename pcl::PointCloud<PointT>::Ptr cloud, float filterRes, Eigen::Vector4f minPoint, Eigen::Vector4f maxPoint)
{

    // Time segmentation process
    auto startTime = std::chrono::steady_clock::now();

    // TODO:: Fill in the function to do voxel grid point reduction and region based filtering
    typename pcl::PointCloud<PointT>::Ptr filteredCloud(new pcl::PointCloud<PointT>());
    // Create the filtering object
    pcl::VoxelGrid<PointT> sor;
    sor.setInputCloud(cloud);
    sor.setLeafSize(filterRes, filterRes, filterRes);
    sor.filter(*filteredCloud);

    std::cerr << "Point cloud after filtering: " << filteredCloud->width * filteredCloud->height << " data points (" << pcl::getFieldsList(*filteredCloud) << ")." << std::endl;

    typename pcl::PointCloud<PointT>::Ptr cloudRegion(new pcl::PointCloud<PointT>());

    pcl::CropBox<PointT> region(true);
    region.setMin(minPoint);
    region.setMax(maxPoint);
    region.setInputCloud(filteredCloud);
    region.filter(*cloudRegion);

    auto endTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "filtering took " << elapsedTime.count() << " milliseconds" << std::endl;

    return cloudRegion;

}


template<typename PointT>
std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> ProcessPointClouds<PointT>::SeparateClouds(pcl::PointIndices::Ptr inliers, typename pcl::PointCloud<PointT>::Ptr cloud) 
{
  // TODO: Create two new point clouds, one cloud with obstacles and other with segmented plane
    typename pcl::PointCloud<PointT>::Ptr obstCloud(new pcl::PointCloud<PointT>());
    typename pcl::PointCloud<PointT>::Ptr planeCloud(new pcl::PointCloud<PointT>());

    for (int index : inliers->indices) {
        planeCloud->points.push_back(cloud->points[index]);
    }
    // Create filtering object
    pcl::ExtractIndices<PointT> extract;
    extract.setInputCloud(cloud);
    extract.setIndices(inliers);
    // extract point cloud corresponding to obstacles
    extract.setNegative(true);
    extract.filter(*obstCloud);

    std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> segResult(obstCloud, planeCloud);

    return segResult;
}


//template<typename PointT>
//std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> ProcessPointClouds<PointT>::SegmentPlane(typename pcl::PointCloud<PointT>::Ptr cloud, int maxIterations, float distanceThreshold)
//{
//    // Time segmentation process
//    auto startTime = std::chrono::steady_clock::now();
//	pcl::PointIndices::Ptr inliers(new pcl::PointIndices());
//    // TODO:: Fill in this function to find inliers for the cloud.
//    pcl::SACSegmentation<PointT> seg;
//
//    seg.setOptimizeCoefficients(true);
//    seg.setModelType(pcl::SACMODEL_PLANE);
//    seg.setMethodType(pcl::SAC_RANSAC);
//    seg.setMaxIterations(maxIterations);
//    seg.setDistanceThreshold(distanceThreshold);
//    seg.setInputCloud(cloud);
//    // Initialize model coefficients
//    pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients());
//    // Get the inliers
//    seg.segment(*inliers, *coefficients);
//    if (inliers->indices.size() == 0) {
//        std::cout << "Could not estimate a planar model for the given dataset." << std::endl;
//    }
//    auto endTime = std::chrono::steady_clock::now();
//    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
//    std::cout << "plane segmentation took " << elapsedTime.count() << " milliseconds" << std::endl;
//
//    std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> segResult = SeparateClouds(inliers,cloud);
//    return segResult;
//}

template<typename PointT>
std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> ProcessPointClouds<PointT>::SegmentPlane(typename pcl::PointCloud<PointT>::Ptr cloud, int maxIterations, float distanceThreshold)
{
    pcl::PointIndices::Ptr inliersResult(new pcl::PointIndices);

    auto startTime = std::chrono::steady_clock::now();

    while (maxIterations--) {
        pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
        while (inliers->indices.size() < 3) inliers->indices.push_back(rand() % cloud->points.size());
        auto it = inliers->indices.begin();
        int idx1 = *it;
        it++;
        int idx2 = *it;
        it++;
        int idx3 = *it;
        float x1 = cloud->points[idx1].x, y1 = cloud->points[idx1].y, z1 = cloud->points[idx1].z;
        float x2 = cloud->points[idx2].x, y2 = cloud->points[idx2].y, z2 = cloud->points[idx2].z;
        float x3 = cloud->points[idx3].x, y3 = cloud->points[idx3].y, z3 = cloud->points[idx3].z;

        float a = (y2 - y1) * (z3 - z1) - (z2 - z1) * (y3 - y1);
        float b = (z2 - z1) * (x3 - x1) - (x2 - x1) * (z3 - z1);
        float c = (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);
        float d = -(a * x1 + b * y1 + c * z1);

        for (int i = 0; i < cloud->points.size(); ++i) {
            if (i == idx1 || i == idx2 || i == idx3) continue;

            PointT point = cloud->points[i];
            float x4 = point.x, y4 = point.y, z4 = point.z;
            float dist = fabs(a * x4 + b * y4 + c * z4 + d) / sqrt(a * a + b * b + c * c);
            if (dist <= distanceThreshold) {
                inliers->indices.push_back(i);
            }
        }
        if (inliers->indices.size() > inliersResult->indices.size()) {
            inliersResult = inliers;
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "RANSAC 3D took: " << elapsedTime.count() << " milliseconds." << std::endl;


    std::pair<typename pcl::PointCloud<PointT>::Ptr, typename pcl::PointCloud<PointT>::Ptr> segResult = SeparateClouds(inliersResult,cloud);
    return segResult;
}


//template<typename PointT>
//std::vector<typename pcl::PointCloud<PointT>::Ptr> ProcessPointClouds<PointT>::Clustering(typename pcl::PointCloud<PointT>::Ptr cloud, float clusterTolerance, int minSize, int maxSize)
//{
//
//    // Time clustering process
//    auto startTime = std::chrono::steady_clock::now();
//
//    std::vector<typename pcl::PointCloud<PointT>::Ptr> clusters;
//
//    // TODO:: Fill in the function to perform euclidean clustering to group detected obstacles
//    // Create KD tree object for the search method of extraction
//    typename pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
//    tree->setInputCloud(cloud);
//
//    std::vector<pcl::PointIndices> clusterIndices;
//    pcl::EuclideanClusterExtraction<PointT> ec;
//    ec.setClusterTolerance(clusterTolerance);
//    ec.setMinClusterSize(minSize);
//    ec.setMaxClusterSize(maxSize);
//    ec.setSearchMethod(tree);
//    ec.setInputCloud(cloud);
//    ec.extract(clusterIndices);
//
//    for (std::vector<pcl::PointIndices>::const_iterator it = clusterIndices.begin(); it != clusterIndices.end(); ++it) {
//        typename pcl::PointCloud<PointT>::Ptr cloudCluster(new pcl::PointCloud<PointT>);
//        for (std::vector<int>::const_iterator pit = it->indices.begin(); pit != it->indices.end(); ++pit) {
//            cloudCluster->points.push_back((*cloud)[*pit]);
//        }
//        cloudCluster->width = cloudCluster->points.size();
//        cloudCluster->height = 1;
//        cloudCluster->is_dense = true;
//
//        std::cout << "Point Cloud representing the cluster: " << cloudCluster->size() << " data points." << std::endl;
//        clusters.push_back(cloudCluster);
//    }
//
//    auto endTime = std::chrono::steady_clock::now();
//    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
//    std::cout << "clustering took " << elapsedTime.count() << " milliseconds and found " << clusters.size() << " clusters" << std::endl;
//
//    return clusters;
//}

template<typename PointT>
void ProcessPointClouds<PointT>::proximity(int idx, const std::vector<std::vector<float>>& points, KdTree* tree, std::vector<int>& cluster, std::vector<bool>& processed, float distanceTol)
{
    processed[idx] = true;
    cluster.push_back(idx);

    std::vector<int> nearbyPoints = tree->search(points[idx], distanceTol);
    for (int i : nearbyPoints) {
        if (!processed[i]) proximity(i, points, tree, cluster, processed, distanceTol);
    }
}

template<typename PointT>
std::vector<std::vector<int>> ProcessPointClouds<PointT>::euclideanCluster(const std::vector<std::vector<float>>& points, KdTree* tree, float distanceTol) {
    std::vector<std::vector<int>> clusters;

    std::vector<bool> processed(points.size(), false);

    for (int i = 0; i < points.size(); ++i) {
        if (!processed[i]) {
            std::vector<int> cluster;
            proximity(i, points, tree, cluster, processed, distanceTol);
            clusters.push_back(cluster);
        }
    }

    return clusters;
}

template<typename PointT>
std::vector<typename pcl::PointCloud<PointT>::Ptr> ProcessPointClouds<PointT>::Clustering(typename pcl::PointCloud<PointT>::Ptr cloud, float clusterTolerance, int minSize, int maxSize)
{
    // Time clustering process
    auto startTime = std::chrono::steady_clock::now();

    std::vector<typename pcl::PointCloud<PointT>::Ptr> clusters;

    // Create a KD tree object for searching nearby points
    KdTree* tree = new KdTree;
    std::vector<std::vector<float>> points;
    // Insert points from cloud in the Kd tree
    int id = 0;
    for (auto point : cloud->points) {
        std::vector<float> p{point.x, point.y, point.z};
        points.push_back(p);
        tree->insert(p, id++);
    }
    std::vector<std::vector<int>> indicesArray = euclideanCluster(points, tree, clusterTolerance);

    for (auto indices : indicesArray) {
        if (indices.size() < minSize || indices.size() > maxSize) continue;

        typename pcl::PointCloud<PointT>::Ptr cluster(new pcl::PointCloud<PointT>());

        for (auto idx : indices) {
            cluster->points.push_back(cloud->points[idx]);
        }

        cluster->width = cluster->points.size();
        cluster->height = 1;
        cluster->is_dense = true;

        clusters.push_back(cluster);
    }

    return clusters;
}


template<typename PointT>
Box ProcessPointClouds<PointT>::BoundingBox(typename pcl::PointCloud<PointT>::Ptr cluster)
{

    // Find bounding box for one of the clusters
    PointT minPoint, maxPoint;
    pcl::getMinMax3D(*cluster, minPoint, maxPoint);
    Box box;
    // For reference, see: http://codextechnicanum.blogspot.com/2015/04/find-minimum-oriented-bounding-box-of.html
//    BoxQ box;
//    Eigen::Vector4f pcaCentroid;
//    pcl::compute3DCentroid(*cluster, pcaCentroid);
//    Eigen::Matrix3f covariance;
//    pcl::computeCovarianceMatrixNormalized(*cluster, pcaCentroid, covariance);
//    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigenSolver(covariance, Eigen::ComputeEigenvectors);
//    Eigen::Matrix3f eigenVectorsPCA = eigenSolver.eigenvectors();
//    eigenVectorsPCA.col(2) = eigenVectorsPCA.col(0).template cross(eigenVectorsPCA.col(1)); //This line is necessary for proper orientation in some cases. The numbers come out the same without it, but
//    //    the signs are different and the box doesn't get correctly oriented in some cases.
//    /* These eigen vectors are used to transform the point cloud to the origin point (0,0,0) such that the eigen vectors
//     * correspond to the axes of the space. The minimum point, maximum point, and the middle of the diagonal between these
//     * two points are calculated for the transformed cloud (also referred to as the projected cloud when using PCL's PCA interface). */
//
//    // Transform the original cloud to the origin where the principal components correspond to the axes.
//    Eigen::Matrix4f projectionTransform(Eigen::Matrix4f::Identity());
//    projectionTransform.template block<3,3>(0,0) = eigenVectorsPCA.transpose();
//    projectionTransform.template block<3,1>(0,3) = -1.f * (projectionTransform.template block<3,3>(0,0) * pcaCentroid.head<3>());
//    typename pcl::PointCloud<PointT>::Ptr cloudPointsProjected(new pcl::PointCloud<PointT>);
//    pcl::transformPointCloud(*cluster, *cloudPointsProjected, projectionTransform);
//    // Get the minimum and maximum points of the transformed cloud
//    PointT minPoint, maxPoint;
//    pcl::getMinMax3D(*cloudPointsProjected, minPoint, maxPoint);
//    const Eigen::Vector3f meanDiagonal = 0.5f*(maxPoint.getVector3fMap() + minPoint.getVector3fMap());
////
////    // Final transform
//    box.bboxQuaternion = eigenVectorsPCA; // Quaternions are a way to do rotations
//    box.bboxTransform = eigenVectorsPCA * meanDiagonal + pcaCentroid.head<3>();
//    box.cube_length = maxPoint.x - minPoint.x;
//    box.cube_height = maxPoint.y - minPoint.y;
//    box.cube_width = maxPoint.z - minPoint.z;
    box.x_min = minPoint.x;
    box.y_min = minPoint.y;
    box.z_min = minPoint.z;
    box.x_max = maxPoint.x;
    box.y_max = maxPoint.y;
    box.z_max = maxPoint.z;

    return box;
}


template<typename PointT>
void ProcessPointClouds<PointT>::savePcd(typename pcl::PointCloud<PointT>::Ptr cloud, std::string file)
{
    pcl::io::savePCDFileASCII (file, *cloud);
    std::cerr << "Saved " << cloud->points.size () << " data points to "+file << std::endl;
}


template<typename PointT>
typename pcl::PointCloud<PointT>::Ptr ProcessPointClouds<PointT>::loadPcd(std::string file)
{

    typename pcl::PointCloud<PointT>::Ptr cloud (new pcl::PointCloud<PointT>);

    if (pcl::io::loadPCDFile<PointT> (file, *cloud) == -1) //* load the file
    {
        PCL_ERROR ("Couldn't read file \n");
    }
    std::cerr << "Loaded " << cloud->points.size () << " data points from "+file << std::endl;

    return cloud;
}


template<typename PointT>
std::vector<boost::filesystem::path> ProcessPointClouds<PointT>::streamPcd(std::string dataPath)
{

    std::vector<boost::filesystem::path> paths(boost::filesystem::directory_iterator{dataPath}, boost::filesystem::directory_iterator{});

    // sort files in accending order so playback is chronological
    sort(paths.begin(), paths.end());

    return paths;

}