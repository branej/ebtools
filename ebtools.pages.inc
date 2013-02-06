<?php
/**
 * @file
 * Ebart tools file with page callback functions
 */

/**
 * Login function
 *
 * login into server, return cookie filename
 *
 * @return
 *   filename of the file storing cookie info
 */
function _ebtools_login() {
  $username  = variable_get('ebtools_username');
  $password  = variable_get('ebtools_password');
  $LOGIN_URL = variable_get('ebtools_login_url');
  $login = array(
    "Username" => $username,
    "Password" => $password,
  );
  $ckfile = tempnam("/tmp", "CURLCOOKIE" . (string)time() . ".txt");
  $ch = curl_init($LOGIN_URL);
  curl_setopt($ch, CURLOPT_COOKIEJAR, $ckfile);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
  curl_setopt($ch, CURLOPT_POST, true);
  curl_setopt($ch, CURLOPT_POSTFIELDS, $login);
  $output = curl_exec($ch);

  return $ckfile;
}

/**
 * General purpose query function
 *
 * Execute Lotus Domino agent/servlet with given
 * query parameters and return results
 *
 * @return array
 * decoded JSON object we got from server
 */
function _ebtools_query($agent, $query_args, $ckfile) {
  $SERVER = variable_get('ebtools_access_url');
  $URL    = $SERVER . '/' . $agent . '?OpenAgent&' . http_build_query($query_args);
  //drupal_set_message(t('Query is !query', array('!query' => "$agent:" . implode('.', $query_args))));
  //drupal_set_message(t('URL:: !url', array('!url' => $URL)));

  $ch = curl_init($URL);
  curl_setopt($ch, CURLOPT_COOKIEFILE, $ckfile);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);

  $output = curl_exec($ch);
  $info = curl_getinfo($ch);
  curl_close($ch);
  $result = json_decode($output, $assoc = TRUE);
  return $result;
}
/**
 * Get all themes for a give person
 */
function _ebtools_person_themes_total($person, $type = 'novina') {
  return _ebtools_theme_total('person', $person, $type);
}

/**
 * Get total by theme for some other query
 */
function _ebtools_theme_total($type, $arg, $dbtype) {
  if (($dbtype != 'novina') && ($dbtype != 'video')) {
    return 0;
  }
  if (in_array($type, array('person', 'place', 'institution', 'party', 'nation'))) {
    $agent = 'by' . $type;
  } else {
    return 0;
  }
  $data = array(
    'dbtype' => $dbtype,
    'theme'  => 1,
  );
  $data[$type] = $arg;

  $min_year = variable_get('ebtools_start_year');
  $max_year = variable_get('ebtools_end_year');
  $themes = [];
  $ckfile = _ebtools_login();
  for ($i=$min_year; $i<=$max_year; $i++) {
    $data['dbyear'] = $i;
    $cid = "ebtools:themes:$arg:$i";
    if (($cache = cache_get($cid)) && !empty($cache->data)) {
      $year_themes = unserialize($cache->data);
    } else {
      $result = _ebtools_query($agent, $data, $ckfile);
      $year_themes = $result['Teme'];
      if ($i < $max_year) {
        $cache_duration = 60*60*24*365; //cache it for entire year, it will probably never change
      } else {
        $cache_duration = 60*10; //lotus updates every 10-15 minutes
      }
      cache_set($cid, serialize($tmp), 'cache', REQUEST_TIME + $cache_duration;
    }
    foreach($year_themes as $theme) {
      if !(in_array($theme, $themes)) {
        $themes[] = $theme;
      }
    }
  }
  unlink($ckfile);
  return $themes;

/**
 * Get total number of search result
 *
 * @return
 * number of result
 */
function _ebtools_pretraga_total($query, $type = 'novina') {
  if (($type != 'novina') && ($type != 'video')) {
    return 0;
  }
  $min_year = variable_get('ebtools_start_year');
  $max_year = variable_get('ebtools_end_year');
  $total  = 0;
  $ckfile = _ebtools_login();
  for ($i=$min_year; $i<=$max_year; $i++) {
    $result = _ebtools_search_yeartotal($query, $i, $ckfile, $type);
    $total += $result;
  }
  unlink($ckfile);
  return $total;
}

/**
 * Get number of search results for given year
 */
function _ebtools_pretraga_yeartotal($query, $year, $type, $ckfile) {
  if ($type == 'novinski') {
    $dbtype = 'novina';
  } elseif ($type == 'video') {
    $dbtype = $type;
  } else {
    return 0;
  }
  $cid = "ebtools:search:$type:yeartotal:$year";
  $data = array(
    "dbtype" => $dbtype,
    "query"  => $query,
    "count"  => 0,
    "total"  => 1,
  );
  if (($cache = cache_get($cid)) && !empty($cache->data)) {
    $num_res = unserialize($cache->data);
  } else {
    $data['dbyear'] = $year;
    $result = _ebtools_query('search', $data, $ckfile);
    $num_res = $result['toplevelentries'];
    //XXX bilo bi bolje da cache istice svaki put sat +10-15 minuta, nakon replikacije
    cache_set($cid, serialize($num_res), 'cache', REQUEST_TIME + 900);
  }
  return $num_res;
}

/**
 * Return search results
 */
function ebtools_pretraga($type, $query) {
  if (($type !== 'novinski') && ($type !== 'video')) {
    $page_array['ebtools_search'] = array(
        '#title' => t('Error'),
        '#markup' => t('Wrong search type'),
      );
    return $page_array;
  } else {
    return _ebtools_pretraga($type, $query);
  }
}

function _ebtools_pretraga($type, $query) {
  if ($type == 'novinski') {
    $dbtype = 'novina';
  } elseif ($type == 'video') {
    $dbtype = $type;
  } else {
    return array();
  }
  $pa_name = 'ebtools_search_' . $type;
  $base = 'ebart/' . $type . '/';

  $min_year = variable_get('ebtools_start_year');
  $max_year = variable_get('ebtools_end_year');
  $num_db = $max_year - $min_year + 1;
  $total = 0;
  $yearly = array();
  $ckfile = _ebtools_login();
  for ($i = $max_year;$i >= $min_year;$i--) {
    $tmp = _ebtools_pretraga_yeartotal($query, $i, $type, $ckfile);
    $yearly[] = $tmp;
    $total += $tmp;
  }

  if ($total > 0) {
    $page = pager_find_page();
    $num_per_page = variable_get('ebtools_count');
    //calculate where do we need to pull data from
    $so_far = $page * $num_per_page;
    if ($so_far > $total) {
      $page_array[$pa_name] = array(
        '#title' => t('Error'),
        '#markup' => t('No more results'),
      );
    } else {
      $data = array(
        'dbtype' => $dbtype,
        'query'  => $query,
        'count'  => $num_per_page,
      );
      $on_db = 0;
      $results_so_far = $yearly[$on_db];
      $start = $so_far + 1;

      while (($results_so_far < $start) && ($on_db < $num_db)) {
        //drupal_set_message("YEARLY[$on_db]\t\tResults:$results_so_far");
        $on_db++;
        $results_so_far += $yearly[$on_db];
      }
      if (($results_so_far >= $start) || ($on_db < $num_db)) {
        $num_res = 0;
        $num_remain = $num_per_page;
        $entries = array();

        if ($start > 0) {
          $start_for_db = $so_far - ($results_so_far - $yearly[$on_db]);
        } else {
          $start_for_db = $so_far;
        }

        while (($num_res < $num_per_page) && ($on_db < $num_db)) {
          $year = $max_year - $on_db;
          //drupal_set_message("on_db:$on_db\tyear:$year");
          $data['dbyear'] = $year;
          $data['start'] = $start_for_db;
          $results = _ebtools_query('search', $data, $ckfile);
          //drupal_set_message('ARGS:: ' . implode(',', $results));
          $tmp = $results['entries'];
          foreach ($tmp as $entry) {
            $ttmp = $entry;
            $ttmp['Godina'] = $year;
            $entries[] = $ttmp;
          }
          $num_remain -= count($tmp);
          $num_res += count($tmp);
          $on_db++;
          $start_for_db = 0;
        }
        foreach ($entries as $entry) {
          //$date = $entry['Dan'] . '.' . $entry['Mesec'] . '.' . $entry['Godina'];
          $tmp = explode('.', $entry['Datum']);
          $date = implode('/', array_reverse($tmp));
          $datelink = 'ebart/news/bydate/' . $date; //$entry['Godina'] . '/' . $entry['Mesec'] . '/' . $entry['Dan'];
          //$url = 'ebart/news/article/' . $entry['Godina'] . '/'. $entry['ID'];
          $url  = $base . 'clanak/' . $tmp[2] . '/'. $entry['unid'];
          $link = $base . 'medij/' . $entry['Novina'];
          $items[] = array(
            'data' => l($entry['Naslov'], $url),
            'children' => array(l($entry['Novina'], $link), l($entry['Datum'], $datelink), $entry['Autor'], $entry['Ukratko']),
          );
        }
        pager_default_initialize($total, $num_per_page);
        $page_array[$pa_name] = array(
          '#title' => t('Showing results @start-@end of @total for "%query"', array(
            '%query' => $query,
            '@start' => $so_far + 1,
            '@end' => min($so_far + $num_per_page, $total),
            '@total' => $total,
          )
        ),
          '#items' => $items,
          '#theme' => 'item_list__ebtools',
          '#attributes' => array(
            'class' => 'articlelist',
          ),
        );
        $page_array['pager'] = array(
          '#markup' => theme('pager'),
          '#theme'  => 'pager__ebtools',
        );
      }
      unlink($ckfile);
    }
  } else {
    $page_array[$pa_name] = array(
      //Title serves as page subtitle
      '#title' => t('No results for "%query".', array('%query' => $query)),
      '#markup' => t('No results.'),
    );
  }
  return $page_array;
}

/**
 * Custom callback function for archives clip page
 */
function _ebtools_archive_clip($godina, $unid) {
  $data = array(
    "dbtype" => "video",
    "dbyear" => $godina,
    "unid"   => $unid);

  $valid_args = TRUE;
  if ((ctype_xdigit($unid) !== TRUE) || (strlen($unid) !== 32) || (ctype_digit($godina) !== TRUE)) {
    $valid_args = FALSE;
  }

  if ($valid_args) {
    $ckfile = _ebtools_login();
    $article = _ebtools_query('getdoc', $data, $ckfile);

    $items = array();
    foreach($article as $field => $value) {
      $f = $field;
      if (is_array($value)) {
        $v = implode(",",$value);
      } else {
        $v = $value;
      }
      $items[] = array($f, $v);
    }
    $header = array('Field', 'Value');
    $page_array['ebtools_archive_clip'] = array(
      '#title' => t('Archive clip view'),
      '#rows' => $items,
      '#header' => $header,
      '#theme' => 'table__ebtools',
    );
  } else {
    $page_array['ebtools_archive_clip'] = array(
      '#title' => t('Error'),
      '#markup' => t('Invalid data'),
    );
  }
  unlink($ckfile);
  return $page_array;
}

//XXX NOVO
/**
 * Return total count of articles for given type and value
 */
function _ebtools_novinski_total($type, $value) {
  $min_year = variable_get('ebtools_start_year');
  $max_year = variable_get('ebtools_end_year');
  $total  = 0;
  $ckfile = _ebtools_login();
  for ($i=$min_year; $i<=$max_year; $i++) {
    $result = _ebtools_novinski_yeartotal($type, $value, $i, $ckfile);
    $total += $result;
  }
  unlink($ckfile);
  return $total;
}

/**
 * Return count of articles for given type, value and year
 */
function _ebtools_novinski_yeartotal($type, $value, $year, $ckfile) {
  if (($type !== 'media') && ($type !== 'place') && ($type !== 'person')) {
    return array();
  }
  $view = 'listby' . $type;
  $data = array(
    'dbtype' => 'novina',
    'count'  => 0,
  );
  $data[$type] = $value;
  $cid = 'ebtools:' . $type . '_total:' . $value . ':' . $year;
  if (($cache = cache_get($cid)) && !empty($cache->data)) {
    $result = unserialize($cache->data);
  } else {
    $data['dbyear'] = $year;
    $tmp    = _ebtools_query($view, $data, $ckfile);
    $result = $tmp['toplevelentries'];
    cache_set($cid, serialize($result), 'cache', REQUEST_TIME + 900);
  }
  return $result;
}

function _ebtools_block_list() {
  $type = &drupal_static('ebtools_block_type');
  $data = _ebtools_all($type);
  $items = array();
  $base  = 'ebart/novinski/';//medij/';
  if ($type == 'media') {
    $base .= 'medij/';
  } elseif ($type == 'place') {
    $base .= 'mesto/';
  } elseif ($type == 'person') {
    $base .= 'licnost/';
  } else {
    return array();
  }
  foreach ($data as $item) {
    $items[] = l($item, $base . $item);
  }
  return $items;
}

function _ebtools_block_listby() {
  $type = &drupal_static('ebtools_block_type');
  $base = 'ebtools_block_by';
  if ($type == 'media') {
    $view = 'medij';
  } elseif ($type == 'place') {
    $view = 'mesto';
  } elseif ($type == 'person') {
    $view = 'licnost';
  } else {
    return array();
  }
  $value = &drupal_static('ebtools_block_value');
  $data = _ebtools_getlist('block', array($view, $value));
  return $data;
}

/**
 * Return list of all of in database
 *
 * Returns a list of $type from database, where $type can be one
 * of media, place, person
 */
function _ebtools_all($type) {
  $view = 'by' . $type;
  if ($type == 'media') {
    $extra_field = 'Novina';
  } elseif ($type == 'place') {
    $extra_field = 'Mesto';
  } elseif ($type == 'person') {
    $extra_field = 'Licnost';
  } else {
    return array();
  }
  $min_year = variable_get('ebtools_start_year');
  $max_year = variable_get('ebtools_end_year');

  $total  = 0;
  $ckfile = _ebtools_login();
  $data = array();
  $values = array();

  for ($i=$min_year; $i<=$max_year; $i++) {
    $cid = 'ebtools:' . $type . 'list:' . $i;
    if (($cache = cache_get($cid)) && !empty($cache->data)) {
      $results = unserialize($cache->data);
    } else {
      $data['dbyear'] = $i;
      // XXX query 'byplace' returns empty field in place list XXX
      $results = array_filter(_ebtools_query($view, $data, $ckfile));
      cache_set($cid, serialize($results), 'cache', REQUEST_TIME + 60*60*24);
    }
    //merge array
    foreach ($results[$extra_field] as $entry) {
      if (!in_array($entry, $values)) {
        $values[] = $entry;
      }
    }
  }
  natcasesort($values);//, SORT_NATURAL | SORT_FLAG_CASE);
  unlink($ckfile);
  return $values;
}

function _ebtools_totable($list, $prefix='') {
  $columns = array();
  $finished = FALSE;

  $i    = 0;
  $ci   = 0;
  $tmp  = array();
  $size = variable_get('ebtools_tablewidth');;
  do {
    $field = $list[$i];
    if ($field !== '') {
      $tmp[] = l($field, $prefix . $field);
      $ci = ($ci + 1) % $size;
    }
    $i++;
    if (($i > 0) && ($ci == 0)) {
      $columns[] = $tmp;
      $tmp = array();
    }
  } while ($i<count($list));
  return $columns;
}


/**
 * Return list of things
 *
 * Returns a list of articles tagged with specific
 * media/place/person or a list of media/places/persona
 */
function _ebtools_list() {
  $args = func_get_args();
  //drupal_set_message("DATA:" . implode('|', $args));
  if ($args[0] == 'mesto') {
    $type = 'place';
  } elseif ($args[0] == 'medij') {
    $type = 'media';
  } elseif ($args[0] == 'licnost') {
    $type = 'person';
  //XXX NOT IMPLEMENTED YET
  } elseif ($args[0] == 'dokument') {
    $type = 'document';
  } elseif ($args[0] == 'manifestacija') {
    $type = 'manifestation';
  } elseif ($args[0] == 'događaj') {
    $type = 'event';
  } elseif ($args[0] == 'stranka') {
    $type = 'party';
  } elseif ($args[0] == 'narod') {
    $type = 'nation';
  } elseif ($args[0] == 'tema') {
    $type = 'theme';
  //XXX NOT IMPLEMENTED YET
  } else {
    //$url = 'ebart/novinski/clanak/' . $godina . '/' . $id;
    drupal_goto($url, array(), 301);
  }
  $block = &drupal_static('ebtools_block_type');
  $block = $type;
  return _ebtools_getlist('page', $args);
}

function _ebtools_getlist($display='page', $args) {
  $numargs   = count($args) - 1;
  $querytype = $args[0];
  //drupal_set_message('DISPLAY: ' . $display . ' GETLIST: ' . join('|', $args));

  $base = 'ebart/novinski/';
  if ($querytype == 'medij') {
    $bylink = $base . 'pomediju/';
    $type = 'media';
  } elseif ($querytype == 'licnost') {
    $bylink = $base . 'policnosti/';
    $type = 'person';
  } elseif ($querytype == 'mesto') {
    $bylink = $base . 'pomestu/';
    $type = 'place';
    //XXX NOT IMPLEMENTED YET
  /*
  } elseif ($querytype == 'dokument') {
    $bylink = $base . 'podokumentu/';
    $type = 'document';
  } elseif ($querytype == 'manifestacija') {
    $bylink = $base . 'pomanifestaciji/';
    $type = 'manifestation';
  } elseif ($querytype == 'događaj') {
    $bylink = $base . 'podogađaju/';
    $type = 'event';
  } elseif ($querytype == 'stranka') {
    $bylink = $base . 'postranci/';
    $type = 'party';
  } elseif ($querytype == 'narod') {
    $bylink = $base . 'ponarodu/';
    $type = 'nation';
  } elseif ($querytype == 'tema') {
    $bylink = $base . 'potemi/';
    $type = 'theme';
    //XXX NOT IMPLEMENTED YET
   */
  } elseif (in_array($querytype, array('dokument', 'manifestacija','događaj', 'stranka', 'narod', 'tema'))) {
    $page_array['ebtools_inform'] = array(
      '#title'  => t('Information'),
      '#markup' => t('Views for type "!type" are not implemented yet', array(
        '!type' => $querytype)
      ),
    );
    return $page_array;
  } else {
    //none of the types we recognize
    $page_array['ebtools_error'] = array(
      '#title'  => t('Error'),
      '#markup' => t('Invalid type "!type"', array(
        '!type' => $querytype)
      ),
    );
    return $page_array;
  }

  $prefix = $base . $querytype .'/';
  $list = _ebtools_all($type);
  $listquery = 'listby' . $type;
  $pa_name = 'ebtools_' .$listquery;


  if ($numargs == 0) {
    $title = t('Listing of !type', array(
      '!type' => $type)
    );
    //return list of $type
    $table = _ebtools_totable($list, $prefix);
    $page_array[$pa_name] = array(
      '#title'   => $title,
      '#rows'    => $table,
      '#theme'   => 'table__ebart',
      //'#tbodyid' => 'listbody',
      '#attributes' => array(
        'id'    => 'listbody',
        'style' => 'width:100%;',
      ),
    );
    drupal_set_title($title);
  } else {
    $value = $args[1];

    if (!in_array($value, $list)) {
      $page_array[$pa_name] = array(
        '#title' => t('Error'),
        '#markup' => t('Invalid !type !value', array(
          '!type'  => $type,
          '!value' => $value)
        ),
      );
      drupal_set_title(t('Error'));
    } else {
      $min_year = variable_get('ebtools_start_year');
      $max_year = variable_get('ebtools_end_year');

      if ($display == 'page') {
        $num_per_page = variable_get('ebtools_count');
      } elseif ($display == 'block') {
        $num_per_page = variable_get('ebtools_bcount');
      } else {
        $page_array[$pa_name] = array(
          '#title' => t('Error'),
          '#markup' => t('Invalid display "!display"', array(
            '!display'  => $display)
          ),
        );
        return $page_array;
      }

      $num_db = $max_year - $min_year + 1;
      $total = 0;
      $yearly = array();
      $ckfile = _ebtools_login();
      for ($i=$max_year;$i>=$min_year;$i--) {
        $tmp = _ebtools_novinski_yeartotal($type, $value, $i, $ckfile);
        $yearly[] = $tmp;
        $total += $tmp;
      }
          //XXX DEBUG
          //$entry = $entries[0];
          /*
          $page_array['ebtools_debug'] = array(
            '#title'  => t('DEBUG'),
            '#markup' => t('DEBUG Message: Total ' . $total),
          );
          return $page_array;
          */
          //XXX DEBUG END
      $page = pager_find_page();

      //calculate where do we need to pull data from
      $so_far = $page * $num_per_page;
      if ($so_far > $total) {
        $page_array[$pa_name] = array(
          '#title' => t('Error'),
          '#markup' => t('No more results'),
        );
      } else {
        $on_db = 0;
        $results_so_far = $yearly[$on_db];
        $start = $so_far + 1;

        while (($results_so_far < $start) && ($on_db < $num_db)) {
          //drupal_set_message("YEARLY[$on_db]\t\tResults:$results_so_far");
          $on_db++;
          $results_so_far += $yearly[$on_db];
        }
        if (($results_so_far >= $start) || ($on_db < $num_db)) {
          $num_res = 0;
          $num_remain = $num_per_page;
          $entries = array();
          $data = array(
            'count' => $num_per_page,
          );
          $data[$type] = $value;

          if ($start > 0) {
            $start_for_db = $so_far - ($results_so_far - $yearly[$on_db]);
          } else {
            $start_for_db = $so_far;
          }

          while (($num_res < $num_per_page) && ($on_db < $num_db)) {
            $year = $max_year - $on_db;
            //drupal_set_message("on_db:$on_db\tyear:$year");
            $data['dbyear'] = $year;
            $data['start'] = $start_for_db;
            $results = _ebtools_query($listquery, $data, $ckfile);
            $tmp = $results['entries'];
            foreach ($tmp as $entry) {
              $ttmp = $entry;
              $ttmp['Godina'] = $year;
              $entries[] = $ttmp;
            }
            $num_remain -= count($tmp);
            $num_res += count($tmp);
            $on_db++;
            $start_for_db = 0;
          }
          /**/
          foreach ($entries as $entry) {
            $date = $entry['Dan'] . '.' . $entry['Mesec'] . '.' . $entry['Godina'];
            $datelink = 'ebart/news/bydate/' . $entry['Godina'] . '/' . $entry['Mesec'] . '/' . $entry['Dan'];
            //$url = 'ebart/news/article/' . $entry['Godina'] . '/'. $entry['ID'];
            $url = $bylink . $value  . '/clanak/' . $entry['Godina'] . '/'. $entry['ID'];
            $link = $base . 'medij/' . $entry['Novina'];
            $items[] = array(
              'data' => l($entry['Naslov'], $url),
              'children' => array(l($entry['Novina'], $link), l($date, $datelink), $entry['ukratko']),
            );
          }
          /**/
          pager_default_initialize($total, $num_per_page);
          $page_array[$pa_name] = array(
            '#title' => t('Showing articles @start-@end of @total for "%value"', array(
              '%value' => $value,
              '@start' => $so_far + 1,
              '@end' => min($so_far + $num_per_page, $total),
              '@total' => $total,
            )
          ),
            '#items' => $items,
            '#theme' => 'item_list__ebtools',
            '#attributes' => array(
              'class' => 'articlelist',
            ),
          );
          $page_array['pager'] = array(
            '#markup' => theme('pager'),
            '#theme'  => 'pager__ebtools',
          );
        }
        unlink($ckfile);
      }
    }
  }
  return $page_array;
}

/**
 * Custom callback function for view by date
 */
function _ebtools_archive_bydate() {
  $numargs = func_num_args();
  $args    = func_get_args();
  $dates   = array("dbyear", "month", "day");
  $data    = array("dbtype" => "novina",);

  $valid_args = TRUE;
  if ($numargs > 0) {
    for ($i=0; $i < $numargs; $i++) {
      if (ctype_digit($args[$i]) == TRUE) {
        $data[$dates[$i]] = $args[$i];
      } else { $valid_args = FALSE;  }
    }
  } else {
    $valid_args = TRUE;
    $args = array(variable_get('ebtools_end_year'));//array('2012'); // XXX promeniti
  }
  if ($valid_args) {
    switch ($numargs) {
      case 0:
      case 1:
        $TITLE = t('List of articles for year !year', array('!year' => $args[0]));
        break;

      case 2:
        $TITLE = t('List of articles for month !month of year !year', array('!month' => $args[1], '!year' => $args[0]));
        break;

      default:
        $TITLE = t('List of articles for !day.!month.!year.', array('!day' => $args[2], '!month' => $args[1], '!year' =>$args[0]));
    }

    $ckfile  = _ebtools_login();
    $article = _ebtools_query('bydate', $data, $ckfile);

    $no_items = FALSE;
    $items = array();


    if ($article['toplevelentries'] > 0) {
      $no_items = TRUE;
    } else {
      switch($numargs) {
      case 0:
      case 1:
        for ($i = 1; $i <= $article['lastmonth']; $i++) {
          $items[] = l('Mesec ' . $i, 'ebart/news/bydate/'. $args[0] . '/' . $i );
        }
        break;

      case 2:
        for ($i = 1; $i <= $article['lastday']; $i++) {
          $items[] = l('Mesec ' . $args[1] . ', dan ' . $i, 'ebart/news/bydate/'. $args[0] . '/' . $args[1] . '/' . $i);
        }
        break;

      case 3:
        $pubdate = implode('.', array_reverse($args));
        foreach ($article['entries'] as $entry) {
          $url = 'ebart/news/article/' . $args[0] . '/'. $entry['ID'];
          $medialink = 'ebart/news/bymedia/' . $args[0] . '/' . $entry['Novina'];
          $items[] = array(
            'data' => l($entry['Naslov'], $url),
            'children' => array(l($entry['Novina'], $medialink), $pubdate, $entry['ukratko']),
          );
        }
      }
    }
    if ($no_items) {
      $page_array['ebtools_articles_bydate'] = array(
        //Title serves as page subtitle
        '#title'  => $TITLE,
        '#markup' => t('No articles for given date.'),
      );
    } else {
      $page_array['ebtools_articles_bydate'] = array(
        '#title' => $TITLE,
        '#items' => $items,
        '#theme' => 'item_list__ebtools',
      );
    }
  } else {
    $page_array['ebtools_archive_bydate'] = array(
      '#title'  => t('Error'),
      '#markup' => t('Invalid data'),
    );
  }
  unlink($ckfile);
  return $page_array;
}

/**
 * helper function to make nicer HTML code for article text
 *
 * since all articles are kept not as paragraphs of text, but as
 * lines of text, we need to mangle it a bit to get regular
 * paragraphs
 *
 */
function _prettify_text($text) {
  $lines = explode("\n", $text);
  $output = "<p>";
  $end = "/\\[.!?]$/";
  for($i=0;$i<count($lines);$i++) {
    $line = trim($lines[$i]);
    if (preg_match($end, $line)) {
      $output .= $line . "</p>\n";
      if ($i < count($lines)) {
        $output .= "<p>";
      }
    } elseif ($line == '') {
      $output .= "</p>\n<p>";
    } else {
      $output .= $line . ' ';
    }
  }
  $output .= "</p>\n";
  return $output;
}
/*
function _ebtools_archive_dump() {
  $args = func_get_args();
  $media = $args[0];
  $godina = $args[1];
  $id = $args[2];
  $page_array = _ebtools_archive_article($godina, $id);
  drupal_set_message('ARGS:  ' . implode('.', $args));

  $data_to_pass = &drupal_static('ebtools_block_bymedia');
  $data_to_pass = $media;
  return $page_array;
}*/

function _ebtools_dump(){//$pregled, $param, $godina, $id) {
  $pregledi = array('pomestu', 'pomediju', 'policnosti');
  $args = func_get_args();
  $godina = $args[2];
  $id = $args[3];
  //drupal_set_message('Params: ' . implode('.', $args));
  if ($args[0] == 'pomestu') {
    $type = 'place';
  } elseif ($args[0] == 'pomediju') {
    $type = 'media';
  } elseif ($args[0] == 'policnosti') {
    $type = 'person';
  } else {
    $url = 'ebart/novinski/clanak/' . $godina . '/' . $id;
    drupal_goto($url, array(), 301);
  }

  $value = &drupal_static('ebtools_block_value');
  $value = $args[1];
  $block = &drupal_static('ebtools_block_type');
  $block = $type;

  $page_array = _ebtools_archive_article($godina, $id);
  return $page_array;
}

/**
 * Function to return raw article data
 *
 * @param
 * $godina - year we are quering
 * $unid - UNID of the wanted article
 *
 * @return
 * page array with article data formated as a table
 *
 */
function _ebtools_archive_rawarticle($godina, $unid) {
  $data = array(
    "dbtype" => "novina",
    "dbyear" => $godina,
    "unid"   => $unid);

  $valid_args = TRUE;
  if ((ctype_xdigit($unid) !== TRUE) || (strlen($unid) !== 32) || (ctype_digit($godina) !== TRUE)) {
    $valid_args = FALSE;
  }

  if ($valid_args) {
    if ($cache = cache_get("ebtools:archive_article:$unid")) {
        $article_page = unserialize($cache->data);
    } else {
      $ckfile  = _ebtools_login();
      $article = _ebtools_query('getdoc', $data, $ckfile);

      $items = array();
      foreach($article as $field=>$value) {
        $f = $field;
        if (is_array($value)) {
          $v = implode(",",$value);
        } else {
          $v = $value;
        }
        $items[] = array($f, $v);
      }
      $header = array('Field', 'Value');
      $article_page['ebtools_archive_rawarticle'] = array(
        '#title'  => t('Raw archive article view'),
        '#rows'   => $items,
        '#header' => $header,
        '#theme'  => 'table__ebtools',
      );
      //$article_page['#title'] = t('@naslov', array('@naslov' => $article['Naslov']));

      //$ARTICLE_PAGE['ebtools_archive_article'] = $article_page;
      cache_set("ebtools:archive_article:$unid", serialize($article_page), 'cache', REQUEST_TIME + 900);
    }
    $page_array['ebtools_archive_article'] = $article_page;
  } else {
    $page_array['ebtools_archive_article'] = array(
      '#title'  => t('Error'),
      '#markup' => t('Invalid data'),
    );
  }
  //delete cookie file
  unlink($ckfile);
  return $page_array;
}

function ebtools_novinski_clanak($godina, $unid) {
  return _ebtools_archive_article($godina, $unid);
}
/**
 * Custom callback function for archives article page
 */
function _ebtools_archive_article($godina, $unid) {
  $data = array(
    "dbtype" => "novina",
    "dbyear" => $godina,
    "unid"   => $unid);

  $valid_args = TRUE;
  if ((ctype_xdigit($unid) !== TRUE) OR 
      (strlen($unid) !== 32) OR
      (ctype_digit($godina) !== TRUE)) {
    $valid_args = FALSE;
  }

  if ($valid_args) {
    $cid = "ebtools:archive_article:$godina:$unid";

    if (($cache = cache_get($cid, 'cache_page')) && !empty($cache->data)) {
      $article_page = unserialize($cache->data);
    } else {
      $ckfile  = _ebtools_login();
      $article = _ebtools_query('getdoc', $data, $ckfile);
      unlink($ckfile);

      $fields = array('Rubrika' => 'RubrikaAA', 'Tema' => 'Teme',);//, 'Mesto');
      $article_page = array(
        '#title' => array(
          t('@naslov', array('@naslov' => $article['Naslov'])),
        ),
      );
      $markup = '';
      foreach($fields as $name => $field) {
        if (array_key_exists($field, $article)) {
          if (is_array($article[$field])) {
            $value = implode(",", $article[$field]);
          } else {
            $value = $article[$field];
          }
          $markup .= "<em>" . $name . "</em>:&nbsp;" . $value . "<br />\n";
        } else {
          $markup .= "<em>" . $name . "</em>:&nbsp;<br />\n";
        }
      }
      /*
      for($i=0; $i<count($fields);$i++) {
        if (array_key_exists($fields[$i], $article)) {
          if (is_array($article[$fields[$i]])) {
            $value = implode(",", $article[$fields[$i]]);
          } else {
            $value = $article[$fields[$i]];
          }
          $markup .= "<em>" . $fields[$i] . "</em>:&nbsp;" . $value . "<br />\n";
        } else {
          $markup .= "<em>" . $fields[$i] . "</em>:&nbsp;<br />\n";
        }
      }
      */
      if (array_key_exists('Mesto', $article)) {
        if (is_array($article['Mesto'])) {
          $tmp = array();
          foreach($article['Mesto'] as $mesto) {
            $tmp[] = l($mesto, 'ebart/novinski/mesto/' . $mesto);
          }
          $value = implode(",", $tmp);
        } else {
          $value = l($article['Mesto'], 'ebart/novinski/mesto/' . $article['Mesto']);
        }
        $markup .= "<em>Mesto</em>:&nbsp;" . $value . "<br />\n";
      }
      $article_page['podaci'] = array(
        '#type'   => 'markup',
        '#markup' => "<p>" . $markup . "</p>\n",
      );
      $article_page['naslov'] = array(
        '#type'   => 'markup',
        '#markup' => '<h3>' . $article['Naslov'] . "</h3>\n",
      );
      //Datum is array (Day, Month, Year)
      $pubdate = implode('.', $article['Datum']);#][0] . '.' . $article['Datum'][1] . '.' . $article['Datum'][2] . '.';
      $publink = 'ebart/news/bydate/' . implode('/', array_reverse($article['Datum']));
      $medialink = 'ebart/novinski/medij/' . $article['Novina'];
      $article_page['objavljeno'] = array(
        '#type'   => 'markup',
        '#markup' => "<p>" . l($pubdate, $publink) . ", " . l($article['Novina'], $medialink) . ", strana " . $article['Strana'] . "</p>\n",
      );
      $article_page['tekst'] = array(
        '#type'   => "markup",
        "#markup" => _prettify_text($article['Tekst']),
        //"#markup" => "<p>" . nl2br($article['Tekst']) . "</p>\n",
      );
      /*
      for($i=0; $i<count($lines);$i++) {
        $line = trim($lines[$i]);
        $tmp = array(
          '#type'   => "markup",
          '#markup' => "<p>" . $line . "</p>\n",
        );
        $article_page['para' . $i] = $tmp;
      }
       */
      if (is_array($article['Autor'])) {
        $value = implode(",", $article['Autor']);
      } else {
        $value = $article['Autor'];
      }
      $article_page['author'] = array(
        '#type'   => 'markup',
        '#markup' => "<p>Autor: " . $value . "</p>\n",
      );
      $antrfile = '';
      foreach($article['Antrfile'] as $ant) {
        $antrfile .= _prettify_text($ant);
      }
      if ($antrfile != '') {
        $article_page['separator'] = array(
          '#type'   => 'markup',
          '#markup' => "<h4 class=\"naslov-antrfile\">Antrfile</h4>\n<hr />\n",
        );
        $article_page['antrfile'] = array(
          '#type'   => 'markup',
          '#markup' => "<div class=\"tekst-antrfile\">" . $antrfile . "</div>\n",
        );
      }
      cache_set($cid, serialize($article_page), 'cache_page', REQUEST_TIME + 900);
    }
    //custom breadcrumbs
    /*$breadcrumb = array();
    $breadcrumb[] = l(t('Home'), '<front>');
    $breadcrumb[] = l(t('Novinski arhiv'), 'ebart/novinski/');
    $breadcrumb[] = l(drupal_get_title(), base_path() . request_uri());
    drupal_set_breadcrumb($breadcrumb);*/
    $page_array['ebtools_archive_article'] = $article_page;
  } else {
    $page_array['ebtools_archive_article'] = array(
      '#title'  => t('Error'),
      '#markup' => t('Invalid data'),
    );
  }
  return $page_array;
}

