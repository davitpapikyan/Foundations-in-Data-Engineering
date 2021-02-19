import org.apache.spark.sql.SparkSession
import org.apache.spark.SparkContext
import org.apache.spark.SparkContext._
import org.apache.spark.SparkConf
import org.apache.spark.sql.types._
import org.apache.spark.sql.functions._
import org.apache.spark.sql.Column
import org.apache.spark.sql.Dataset
import org.apache.spark.sql.Row

object ReturnTrips {

  /**
   *
   * @param A_longitude - The longitude of start point in randians.
   * @param A_latitude - The latitude of start point in randians.
   * @param B_longitude - The longitude of end point in randians.
   * @param B_latitude - The longitude of start point in randians.
   * @return - The haversine distance between A and B.
   */
  def haversine_distance(A_longitude: Column, A_latitude: Column,
                         B_longitude: Column, B_latitude: Column) : Column = {
    val r = lit(6371000.0);  // Earth radius in meters.
    val h = pow(sin((B_latitude-A_latitude)/2.0), 2) +
      pow(sin((B_longitude-A_longitude)/2.0), 2) * cos(A_latitude) * cos(B_latitude)
    lit(2.0) * r * atan2(sqrt(h), sqrt(lit(1.0)-h))
  }

  def compute(trips : Dataset[Row], dist : Double, spark : SparkSession) : Dataset[Row] = {

    import spark.implicits._

    val main_data = trips.select(
      $"tpep_pickup_datetime".as("pickup_datetime"),
      $"tpep_dropoff_datetime".as("dropoff_datetime"),
      $"pickup_longitude", $"pickup_latitude",
      $"dropoff_longitude", $"dropoff_latitude")
      .withColumn("pickup_datetime", unix_timestamp($"pickup_datetime"))
      .withColumn("dropoff_datetime", unix_timestamp($"dropoff_datetime"))
      .withColumn("pickup_longitude", radians($"pickup_longitude"))
      .withColumn("pickup_latitude", radians($"pickup_latitude"))
      .withColumn("dropoff_longitude", radians($"dropoff_longitude"))
      .withColumn("dropoff_latitude", radians($"dropoff_latitude"))

    val max_latitude_pickup = main_data.agg(abs(max($"pickup_latitude"))).first.getDouble(0)
    val max_latitude_dropoff = main_data.agg(abs(max($"dropoff_latitude"))).first.getDouble(0)
    val mal_latitude = math.max(max_latitude_pickup, max_latitude_dropoff)
    // [dist] meters corresponds to [dist / (r * cos(phi))] radians of longitude.
    // See the link -> https://en.wikipedia.org/wiki/Longitude#Length_of_a_degree_of_longitude
    val dist_radian_longitude = dist / (6371000.0 * math.cos(mal_latitude))

    val main_data_bucketized = main_data
      .withColumn("pickup_datetime_bucketized", floor($"pickup_datetime"/28800))
      .withColumn("dropoff_datetime_bucketized", floor($"dropoff_datetime"/28800))
      .withColumn("pickup_longitude_bucketized", floor($"pickup_longitude"/dist_radian_longitude))
      .withColumn("dropoff_longitude_bucketized", floor($"dropoff_longitude"/dist_radian_longitude))
      .sort("dropoff_datetime_bucketized")

    val main_data_exploded = main_data_bucketized
      .withColumn("pickup_datetime_bucketized",
        explode(array($"pickup_datetime_bucketized"-1, $"pickup_datetime_bucketized", $"pickup_datetime_bucketized"+1)))
      .withColumn("pickup_longitude_bucketized",
        explode(array($"pickup_longitude_bucketized"-1, $"pickup_longitude_bucketized", $"pickup_longitude_bucketized"+1)))
      .withColumn("dropoff_longitude_bucketized",
        explode(array($"dropoff_longitude_bucketized"-1, $"dropoff_longitude_bucketized", $"dropoff_longitude_bucketized"+1)))
      .sort("dropoff_datetime_bucketized")

    main_data_bucketized.as("a").join(main_data_exploded.as("b"),
      ($"a.dropoff_datetime_bucketized" === $"b.pickup_datetime_bucketized") &&
        ($"a.dropoff_longitude_bucketized" === $"b.pickup_longitude_bucketized") &&
        ($"a.pickup_longitude_bucketized" === $"b.dropoff_longitude_bucketized") &&
        ($"a.dropoff_datetime" < $"b.pickup_datetime") &&
        ($"a.dropoff_datetime" + 28800 > $"b.pickup_datetime") &&
        (haversine_distance($"a.pickup_longitude", $"a.pickup_latitude",
          $"b.dropoff_longitude", $"b.dropoff_latitude") < dist) &&
        (haversine_distance($"a.dropoff_longitude", $"a.dropoff_latitude",
          $"b.pickup_longitude", $"b.pickup_latitude") < dist))
  }
}