# Closest point

Calculate closest point on geometry given x,y position 

![closest-point](https://user-images.githubusercontent.com/59056/30486031-3b5f8d82-9a27-11e7-8911-9311d28049b8.gif)


## Requirements

* `boost_1_65`

* `boost::geometry` from https://github.com/boostorg/geometry ( `git clone git@github.com:boostorg/geometry.git`) with the following patch applied

 ```patch
 
diff --git a/include/boost/geometry/extensions/strategies/cartesian/distance_info.hpp b/include/boost/geometry/extensions/strategies/cartesian/distance_info.hpp
index 0f442de15..df1a32b8c 100644
--- a/include/boost/geometry/extensions/strategies/cartesian/distance_info.hpp
+++ b/include/boost/geometry/extensions/strategies/cartesian/distance_info.hpp
@@ -168,6 +168,9 @@ public :
                     = c1 < zero ? apply_point_point(p, p1)
                     : c1 > c2 ? apply_point_point(p, p2)
                     : result.projected_distance1;
+
+        if (c1 < zero ) result.projected_point1 = p1;
+        else if (c1 > c2) result.projected_point1 = p2;
     }
 };

```

